#include "serde.h"

FunctionCallInfo gpc_create_fcinfo_with_flinfo(short nargs) {
  FunctionCallInfo fcinfo =
      palloc0(sizeof(FunctionCallInfoData) + sizeof(FmgrInfo));
  fcinfo->flinfo = (FmgrInfo *)((char *)fcinfo + sizeof(FmgrInfo));
  fcinfo->nargs = nargs;
  for (int i = 0; i < fcinfo->nargs; i++) fcinfo->argnull[i] = false;
  // FIXME: Is it appropriate to use CurrentMemoryContext here?
  fcinfo->flinfo->fn_mcxt = CurrentMemoryContext;
  return fcinfo;
}

struct gpc_serde_info *gpc_serde_create_info(short nargs) {
  struct gpc_serde_info *fcinfos =
      (struct gpc_serde_info *)palloc(sizeof(struct gpc_serde_info));
  fcinfos->proc_fcinfo = gpc_create_fcinfo_with_flinfo(nargs);
  fcinfos->args_fcinfo = gpc_create_fcinfo_with_flinfo(nargs);
  fcinfos->types_fcinfo = gpc_create_fcinfo_with_flinfo(nargs);
  fcinfos->rels_fcinfo = gpc_create_fcinfo_with_flinfo(nargs);
  fcinfos->attrs_fcinfo = gpc_create_fcinfo_with_flinfo(nargs);
  fcinfos->oids_fcinfo = gpc_create_fcinfo_with_flinfo(1);
  return fcinfos;
}

void gpc_serde_free_info(struct gpc_serde_info *fcinfos) {
  pfree(fcinfos->proc_fcinfo);
  pfree(fcinfos->args_fcinfo);
  pfree(fcinfos->types_fcinfo);
  pfree(fcinfos->rels_fcinfo);
  pfree(fcinfos->attrs_fcinfo);

  pfree(fcinfos);
}

// FIXME:
// 1. This proction does not consider endianess.
//
// 2. fn_expr, context and resultinfo are not included. It seems that we only
// need type and expectedDesc in resultinfo. Therefore, I choose to rebuild
// resultinfo in the server side instead of sending it.
bytea *gpc_serde_send_fcinfo(PG_FUNCTION_ARGS) {
  int32 args_size = fcinfo->nargs * (sizeof(Datum) + sizeof(bool));
  int32 fcinfo_size = offsetof(FunctionCallInfoData, arg);
  int32 size = VARHDRSZ + fcinfo_size + args_size + sizeof(FmgrInfo);
  bytea *bytes = (bytea *)palloc(size);
  SET_VARSIZE(bytes, size);
  memcpy(VARDATA(bytes), fcinfo, fcinfo_size);
  memcpy(VARDATA(bytes) + fcinfo_size + args_size, fcinfo->flinfo, sizeof(FmgrInfo));
  ((FunctionCallInfo)VARDATA(bytes))->flinfo = NULL;
  return bytes;
}

FunctionCallInfo gpc_serde_recv_fcinfo(bytea *bytes) {
  int32 fcinfo_size = offsetof(FunctionCallInfoData, arg);
  FunctionCallInfo fcinfo = (FunctionCallInfo)VARDATA(bytes);
  int32 args_size = fcinfo->nargs * (sizeof(Datum) + sizeof(bool));
  fcinfo->flinfo = (FmgrInfo *)((char *)fcinfo + fcinfo_size + args_size);
  return fcinfo;
}

ArrayType *gpc_copy_tuples_to_array(struct gpc_vector *tuples,
                                    TupleDesc tup_desc) {
  if (tuples == NULL) return NULL;

  int elmlen = -1;
  bool elmbyval = false;
  char elmalign = 'd';
  HeapTupleHeader *elems =
      (HeapTupleHeader *)palloc(sizeof(HeapTupleHeader) * tuples->size);
  for (Size i = 0; i < tuples->size; i++) {
    elems[i] = (HeapTupleHeader)heap_copy_tuple_as_datum(
        (HeapTuple)(tuples->elems[i]), tup_desc);
  }
  ArrayType *tup_array =
      construct_array((Datum *)elems, tuples->size, tup_desc->tdtypeid, elmlen,
                      elmbyval, elmalign);
  for (Size i = 0; i < tuples->size; i++) {
    pfree(elems[i]);
  }
  pfree(elems);
  return tup_array;
}

bytea *gpc_serde_send_tuple(HeapTuple tuples, TupleDesc desc,
                            FunctionCallInfo fcinfo) {
  Datum datum = heap_copy_tuple_as_datum((HeapTuple)tuples, desc);
  fcinfo->arg[0] = datum;
  return DatumGetByteaP(record_send(fcinfo));
}

bytea *gpc_serde_send_tuple_array(struct gpc_vector *tuples, TupleDesc desc,
                                  FunctionCallInfo fcinfo) {
  Datum datum =
      (Datum)gpc_copy_tuples_to_array((struct gpc_vector *)tuples, desc);
  fcinfo->arg[0] = datum;
  return DatumGetByteaP(array_send(fcinfo));
}

StringInfo gpc_serde_bytea_to_sinfo(bytea *bytes) {
  StringInfo sinfo = palloc(sizeof(StringInfoData) + VARSIZE(bytes));
  sinfo->cursor = 0;
  sinfo->len = VARSIZE(bytes) - VARHDRSZ;
  sinfo->maxlen = sinfo->len + 1;
  sinfo->data = VARDATA(bytes);
  return sinfo;
}

HeapTuple gpc_serde_recv_tuple(bytea *bytes, Oid type_id, int32 type_mod,
                               FunctionCallInfo fcinfo) {
  elog(WARNING, "recv tuple type id = %d", type_id);
  StringInfo sinfo = gpc_serde_bytea_to_sinfo(bytes);
  fcinfo->arg[0] = PointerGetDatum(sinfo);
  fcinfo->arg[1] = type_id;
  fcinfo->arg[2] = type_mod;
  HeapTupleHeader tup_header = DatumGetHeapTupleHeader(record_recv(fcinfo));
  HeapTuple tup = palloc0(sizeof(HeapTupleData));
  tup->t_len = HeapTupleHeaderGetDatumLength(tup_header);
  tup->t_data = tup_header;
  return tup;
}

ArrayType *gpc_serde_recv_tuple_array(bytea *bytes, Oid type_id, int32 type_mod,
                                      FunctionCallInfo fcinfo) {
  StringInfo sinfo = gpc_serde_bytea_to_sinfo(bytes);
  fcinfo->arg[0] = PointerGetDatum(sinfo);
  fcinfo->arg[1] = type_id;
  fcinfo->arg[2] = type_mod;
  return DatumGetArrayTypeP(array_recv(fcinfo));
}

struct gpc_vector *gpc_serde_create_tuples_from_array(ArrayType *array) {
  int init_capacity = 6;
  double scale_factor = 2.5;
  struct gpc_vector *vector = gpc_vector_create(init_capacity, scale_factor);
  ArrayIterator iter = array_create_iterator(array, 0);
  Datum value = 0;
  bool is_null = true;
  while (array_iterate(iter, &value, &is_null)) {
    HeapTuple tup = palloc0(sizeof(HeapTupleData));
    tup->t_len = HeapTupleHeaderGetDatumLength(value);
    tup->t_data = DatumGetHeapTupleHeader(value);
    gpc_vector_push_back(vector, tup);
  }
  array_free_iterator(iter);
  return vector;
}

bytea *gpc_serde_send_tuple_oids(struct gpc_vector *tuples,
                                 FunctionCallInfo fcinfo) {
  Oid *oids = palloc(sizeof(Oid) * tuples->size);
  for (Size i = 0; i < tuples->size; i++) {
    oids[i] = HeapTupleGetOid((HeapTuple)(tuples->elems[i]));
  }
  oidvector *oid_vec = buildoidvector(oids, tuples->size);
  fcinfo->arg[0] = PointerGetDatum(oid_vec);
  return DatumGetByteaP(oidvectorsend(fcinfo));
}

void gpc_serde_recv_tuple_oids(bytea *bytes, struct gpc_vector *tuples,
                               FunctionCallInfo fcinfo) {
  StringInfo sinfo = gpc_serde_bytea_to_sinfo(bytes);
  fcinfo->arg[0] = PointerGetDatum(sinfo);
  oidvector *oids = (oidvector *)oidvectorrecv(fcinfo);
  for (Size i = 0; i < tuples->size; i++) {
    HeapTupleSetOid((HeapTuple)(tuples->elems[i]), oids->values[i]);
  }
}
