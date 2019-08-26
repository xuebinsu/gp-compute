#include "call.h"

enum gpc_status gpc_parse_type(Oid type_id, int32 type_mod,
                               struct gpc_vector *types,
                               struct gpc_vector *rels,
                               struct gpc_vector *attrs,
                               TupleDesc pg_attribute_desc) {
  if (type_id == InvalidOid) return GPC_INVALID_ARGS;

  HeapTuple type_tup = SearchSysCacheCopy1(TYPEOID, type_id);
  if (!HeapTupleIsValid(type_tup)) return GPC_TYPE_NOT_FOUND;
  gpc_vector_push_back(types, type_tup);

  Form_pg_type type = (Form_pg_type)GETSTRUCT(type_tup);
  if (type->typelem != InvalidOid) {
    return gpc_parse_type(type->typelem, -1, types, rels, attrs,
                          pg_attribute_desc);
  }
  if (type->typrelid != InvalidOid || type_mod >= 0) {
    // FIXME:
    // 1. handle anonymous composite types (i.e., composite types not
    // defined in pg_class) correctly. The attrelid = 0 for attributes of
    // anonymous composite types and the tuple descriptor is stored in
    // RecordCacheArray.
    //
    // 2. Polymorphic types are not supported such as Any, AnyElement and
    // AnyArray so that we do not need to send fn_expr.
    TupleDesc tup_desc = lookup_rowtype_tupdesc(type_id, type_mod);
    for (int j = 0; j < tup_desc->natts; j++) {
      // TODO: build a cache for tuples in pg_attribute for better performance.
      HeapTuple attr_tup = gpc_catalog_create_tuple_pg_attr(pg_attribute_desc);
      memcpy(GETSTRUCT(attr_tup), tup_desc->attrs[j],
             sizeof(FormData_pg_attribute));
      gpc_vector_push_back(attrs, attr_tup);
      gpc_parse_type(tup_desc->attrs[j]->atttypid, -1, types, rels, attrs,
                     pg_attribute_desc);
    }
    if (tup_desc->natts > 0 && tup_desc->attrs[0]->attrelid != InvalidOid) {
      HeapTuple rel_tup =
          SearchSysCacheCopy1(RELOID, tup_desc->attrs[0]->attrelid);
      if (!HeapTupleIsValid(type_tup)) return GPC_REL_NOT_FOUND;
      gpc_vector_push_back(rels, rel_tup);
    }
  }

  return GPC_SUCCESS;
}

enum gpc_status gpc_call_get_proc(struct gpc_call *call, PG_FUNCTION_ARGS) {
  call->fcinfo = fcinfo;
  HeapTuple proc_tup =
      SearchSysCacheCopy1(PROCOID, ObjectIdGetDatum(fcinfo->flinfo->fn_oid));
  if (!HeapTupleIsValid(proc_tup)) return GPC_PROC_NOT_FOUND;
  call->proc = proc_tup;
  return GPC_SUCCESS;
}

enum gpc_status gpc_call_get_args_desc(struct gpc_call *call) {
  if (call->fcinfo == NULL || call->proc == NULL) return GPC_INVALID_ARGS;

  call->args_desc = CreateTemplateTupleDesc(call->fcinfo->nargs, false);
  Form_pg_proc proc = (Form_pg_proc)GETSTRUCT(call->proc);
  for (int i = 0; i < call->fcinfo->nargs; i++) {
    TupleDescInitEntry(call->args_desc, i + 1, NULL,
                       proc->proargtypes.values[i], -1, 0);
  }
  assign_record_type_typmod(call->args_desc);
  return GPC_SUCCESS;
}

enum gpc_status gpc_call_get_args_tuple(struct gpc_call *call) {
  if (call->args_desc == NULL) return GPC_INVALID_ARGS;
  call->args = heap_form_tuple(call->args_desc, call->fcinfo->arg,
                               call->fcinfo->argnull);
  return GPC_SUCCESS;
}

enum gpc_status gpc_call_get_args_types(struct gpc_call *call) {
  if (call->args_desc == NULL) return GPC_INVALID_ARGS;
  return gpc_parse_type(RECORDOID, call->args_desc->tdtypmod, call->types,
                        call->rels, call->attrs, call->pg_attribute_desc);
}

enum gpc_status gpc_call_get_ret_type(PG_FUNCTION_ARGS, struct gpc_call *call) {
  get_call_result_type(fcinfo, &call->ret_type, &call->ret_desc);
  if (call->ret_desc != NULL) {
    call->ret_desc->tdtypeid = RECORDOID;
    assign_record_type_typmod(call->ret_desc);
  }
  int32 ret_type_mod = call->ret_desc != NULL ? call->ret_desc->tdtypmod : -1;
  return gpc_parse_type(call->ret_type, ret_type_mod, call->types, call->rels,
                        call->attrs, call->pg_attribute_desc);
}

int gpc_cmp_by_oid(const void *lhs, const void *rhs) {
  Oid lid = HeapTupleGetOid(*(HeapTuple *)lhs);
  Oid rid = HeapTupleGetOid(*(HeapTuple *)rhs);
  if (lid == rid) return 0;
  return lid < rid ? -1 : 1;
}

int gpc_cmp_attrs(const void *lhs, const void *rhs) {
  Form_pg_attribute lattr = (Form_pg_attribute)GETSTRUCT(*(HeapTuple *)lhs);
  Form_pg_attribute rattr = (Form_pg_attribute)GETSTRUCT(*(HeapTuple *)rhs);
  if (lattr->attrelid == rattr->attrelid) {
    if (lattr->attnum == rattr->attnum) return 0;
    return lattr->attnum < rattr->attnum ? -1 : 1;
  }
  return lattr->attrelid < rattr->attrelid ? -1 : 1;
}

struct gpc_call *gpc_call_create(void) {
  struct gpc_call *call = palloc0(sizeof(struct gpc_call));
  int init_capacity = 6;
  double scale_factor = 2.5;
  call->types = gpc_vector_create(init_capacity, scale_factor);
  call->rels = gpc_vector_create(init_capacity, scale_factor);
  call->attrs = gpc_vector_create(init_capacity, scale_factor);
  call->pg_proc_desc =
      lookup_rowtype_tupdesc_copy(ProcedureRelation_Rowtype_Id, -1);
  call->pg_type_desc = lookup_rowtype_tupdesc_copy(TypeRelation_Rowtype_Id, -1);
  call->pg_class_desc =
      lookup_rowtype_tupdesc_copy(RelationRelation_Rowtype_Id, -1);
  call->pg_attribute_desc =
      lookup_rowtype_tupdesc_copy(AttributeRelation_Rowtype_Id, -1);
  return call;
}

void gpc_call_distinct_tuples(struct gpc_call *call) {
// FIXME: The old vector and its elements are not freed.
#define DISTINCT_AND_REPLACE(VEC, CMP)                               \
  do {                                                               \
    struct gpc_vector *DISTINCT_VEC = gpc_vector_distinct(VEC, CMP); \
    VEC = DISTINCT_VEC;                                              \
  } while (0)

  DISTINCT_AND_REPLACE(call->types, gpc_cmp_by_oid);
  DISTINCT_AND_REPLACE(call->rels, gpc_cmp_by_oid);
  DISTINCT_AND_REPLACE(call->types, gpc_cmp_attrs);
}

bytea *gpc_call_send(struct gpc_call *call, struct gpc_serde_info *send_info) {
  if (send_info == NULL || call == NULL) return NULL;

  bytea *fcinfo_bytes = gpc_serde_send_fcinfo(call->fcinfo);
  bytea *proc_bytes = gpc_serde_send_tuple(call->proc, call->pg_proc_desc,
                                           send_info->proc_fcinfo);
  bytea *args_bytes =
      gpc_serde_send_tuple(call->args, call->args_desc, send_info->args_fcinfo);

  bytea *types_bytes = gpc_serde_send_tuple_array(
      call->types, call->pg_type_desc, send_info->types_fcinfo);
  bytea *rels_bytes = gpc_serde_send_tuple_array(
      call->rels, call->pg_class_desc, send_info->rels_fcinfo);
  bytea *attrs_bytes = gpc_serde_send_tuple_array(
      call->attrs, call->pg_attribute_desc, send_info->attrs_fcinfo);

  bytea *type_ids_bytes =
      gpc_serde_send_tuple_oids(call->types, send_info->oids_fcinfo);
  bytea *rel_ids_bytes =
      gpc_serde_send_tuple_oids(call->rels, send_info->oids_fcinfo);

  int size = VARHDRSZ + VARSIZE(fcinfo_bytes) + VARSIZE(proc_bytes) +
             VARSIZE(args_bytes) + VARSIZE(types_bytes) + VARSIZE(rels_bytes) +
             VARSIZE(attrs_bytes) + VARSIZE(type_ids_bytes) +
             VARSIZE(rel_ids_bytes);
  bytea *call_bytes = (bytea *)palloc(size);
  SET_VARSIZE(call_bytes, size);
  char *cur = VARDATA(call_bytes);

#define COPY_BYTES(CUR, SRC)                                           \
  do {                                                                 \
    memcpy(CUR, SRC, VARSIZE(SRC));                                    \
    elog(WARNING, "call send cur = %p, size = %d", cur, VARSIZE(SRC)); \
    CUR += VARSIZE(SRC);                                               \
  } while (0)

  COPY_BYTES(cur, fcinfo_bytes);
  COPY_BYTES(cur, proc_bytes);
  COPY_BYTES(cur, args_bytes);
  COPY_BYTES(cur, types_bytes);
  COPY_BYTES(cur, rels_bytes);
  COPY_BYTES(cur, attrs_bytes);
  COPY_BYTES(cur, type_ids_bytes);
  COPY_BYTES(cur, rel_ids_bytes);

  return call_bytes;
}

void gpc_call_free(struct gpc_call *call) {
  // NOTE: We do not need to free fcinfo here. It will be freed by whoever calls
  // the language handler.
  pfree(call->proc);
  pfree(call->args);
  gpc_vector_free(call->types);
  gpc_vector_free(call->rels);
  gpc_vector_free(call->attrs);

  pfree(call->pg_proc_desc);
  pfree(call->args_desc);
  pfree(call->pg_type_desc);
  pfree(call->pg_class_desc);
  pfree(call->pg_attribute_desc);

  pfree(call->call_bytes);

  // NOTE: we do not need to free ret_type since it is passed by value.
  if (call->ret_desc != NULL) pfree(call->ret_desc);
  if (call->ret_bytes != NULL) pfree(call->ret_bytes);

  pfree(call);
}

struct gpc_call *gpc_call_recv(bytea *bytes, struct gpc_serde_info *recv_info) {
  elog(WARNING, "call bytes size = %d", VARSIZE(bytes));
  struct gpc_call *call = palloc0(sizeof(struct gpc_call));
  call->call_bytes = bytes;
  char *cur = VARDATA(call->call_bytes);

#define EXTRACT_BYTES(CUR, DEST)                           \
  bytea *DEST = (bytea *)CUR;                              \
  elog(WARNING, "extract bytes size = %d", VARSIZE(DEST)); \
  CUR += VARSIZE(DEST)

  EXTRACT_BYTES(cur, fcinfo_bytes);
  EXTRACT_BYTES(cur, proc_bytes);
  EXTRACT_BYTES(cur, args_bytes);
  EXTRACT_BYTES(cur, types_bytes);
  EXTRACT_BYTES(cur, rels_bytes);
  EXTRACT_BYTES(cur, attrs_bytes);
  EXTRACT_BYTES(cur, type_ids_bytes);
  EXTRACT_BYTES(cur, rel_ids_bytes);

  call->fcinfo = gpc_serde_recv_fcinfo(fcinfo_bytes);

  call->proc = gpc_serde_recv_tuple(proc_bytes, ProcedureRelation_Rowtype_Id,
                                    -1, recv_info->proc_fcinfo);

  gpc_call_get_args_desc(call);
  elog(WARNING, "hahaha");

  call->args =
      gpc_serde_recv_tuple(args_bytes, call->args_desc->tdtypeid,
                           call->args_desc->tdtypmod, recv_info->args_fcinfo);

  // FIXME: The following arrays are not freed.
  ArrayType *type_array = gpc_serde_recv_tuple_array(
      types_bytes, TypeRelation_Rowtype_Id, -1, recv_info->types_fcinfo);
  call->types = gpc_serde_create_tuples_from_array(type_array);

  ArrayType *rel_array = gpc_serde_recv_tuple_array(
      rels_bytes, RelationRelation_Rowtype_Id, -1, recv_info->rels_fcinfo);
  call->rels = gpc_serde_create_tuples_from_array(rel_array);

  ArrayType *attr_array = gpc_serde_recv_tuple_array(
      attrs_bytes, AttributeRelation_Rowtype_Id, -1, recv_info->attrs_fcinfo);
  call->attrs = gpc_serde_create_tuples_from_array(attr_array);

  gpc_serde_recv_tuple_oids(type_ids_bytes, call->types,
                            recv_info->oids_fcinfo);
  gpc_serde_recv_tuple_oids(rel_ids_bytes, call->rels, recv_info->oids_fcinfo);

  return call;
}

enum gpc_status gpc_call_setup(struct gpc_call *call) {
  HeapTupleSetOid(call->proc, call->fcinfo->flinfo->fn_oid);
  gpc_catalog_cache_insert_by_oid(PROCOID, call->proc,
                                  call->fcinfo->flinfo->fn_oid);
  heap_deform_tuple(call->args, call->args_desc, call->fcinfo->arg,
                    call->fcinfo->argnull);
  for (Size i = 0; i < call->types->size; i++) {
    gpc_catalog_cache_insert_by_oid(TYPEOID, (HeapTuple)(call->types->elems[i]),
                                    InvalidOid);
  }
  for (Size i = 0; i < call->rels->size; i++) {
    gpc_catalog_cache_insert_by_oid(RELOID, (HeapTuple)(call->rels->elems[i]),
                                    InvalidOid);
  }
  Form_pg_proc proc = (Form_pg_proc)GETSTRUCT(call->proc);
  call->ret_type = proc->prorettype;
  HeapTuple ret_tup = SearchSysCache1(TYPEOID, call->ret_type);
  if (!HeapTupleIsValid(ret_tup)) return GPC_TYPE_NOT_FOUND;
  Oid ret_rel_id = ((Form_pg_type)GETSTRUCT(ret_tup))->typrelid;
  ReleaseSysCache(ret_tup);
  int init_capacity = 6;
  double scale_factor = 2.5;
  struct gpc_vector *ret_attrs = gpc_vector_create(init_capacity, scale_factor);
  for (Size i = 0; i < call->attrs->size; i++) {
    Form_pg_attribute attr =
        (Form_pg_attribute)GETSTRUCT((HeapTuple)call->attrs->elems[i]);
    if (attr->attrelid != InvalidOid) {
      gpc_catalog_cache_insert_attr((HeapTuple)call->attrs->elems[i]);
    }
    if (attr->attrelid == ret_rel_id) {
      gpc_vector_push_back(ret_attrs, attr);
    }
  }
  if (ret_attrs->size > 0) {
    call->ret_desc = CreateTupleDesc(ret_attrs->size, true,
                                     (Form_pg_attribute *)ret_attrs->elems);
    if (ret_rel_id == InvalidOid) {
      assign_record_type_typmod(call->ret_desc);
    }
    call->fcinfo->resultinfo = (fmNodePtr)palloc0(sizeof(ReturnSetInfo));
    ((ReturnSetInfo *)call->fcinfo->resultinfo)->expectedDesc = call->ret_desc;
  }
  // TODO: Insert a pg_language tuple for the function.
  return GPC_SUCCESS;
}
