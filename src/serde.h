#pragma once

#include "postgres.h"
#include "fmgr.h"

#include "access/tupdesc.h"
#include "access/htup.h"
#include "access/htup_details.h"
#include "nodes/execnodes.h"
#include "utils/array.h"
#include "utils/builtins.h"

#include "vector.h"

struct gpc_serde_info {
  FunctionCallInfo proc_fcinfo;
  FunctionCallInfo args_fcinfo;
  FunctionCallInfo types_fcinfo;
  FunctionCallInfo rels_fcinfo;
  FunctionCallInfo attrs_fcinfo;
  FunctionCallInfo oids_fcinfo;
};

FunctionCallInfo gpc_create_fcinfo_with_flinfo(short nargs);

struct gpc_serde_info *gpc_serde_create_info(short nargs);

void gpc_serde_free_info(struct gpc_serde_info *fcinfos);

ArrayType *gpc_copy_tuples_to_array(struct gpc_vector *tuples,
                                    TupleDesc tup_desc);

bytea *gpc_serde_send_fcinfo(PG_FUNCTION_ARGS);

FunctionCallInfo gpc_serde_recv_fcinfo(bytea *bytes);

bytea *gpc_serde_send_tuple(HeapTuple tuple, TupleDesc desc,
                            FunctionCallInfo fcinfo);

bytea *gpc_serde_send_tuple_array(struct gpc_vector *tuples, TupleDesc desc,
                                  FunctionCallInfo fcinfo);

StringInfo gpc_serde_bytea_to_sinfo(bytea *bytes);

HeapTuple gpc_serde_recv_tuple(bytea *bytes, Oid type_id, int32 type_mod,
                               FunctionCallInfo fcinfo);

ArrayType *gpc_serde_recv_tuple_array(bytea *bytes, Oid type_id, int32 type_mod,
                                      FunctionCallInfo fcinfo);

struct gpc_vector *gpc_serde_create_tuples_from_array(ArrayType *array);

bytea *gpc_serde_send_tuple_oids(struct gpc_vector *tuples, FunctionCallInfo fcinfo);

void gpc_serde_recv_tuple_oids(bytea *bytes, struct gpc_vector *tuples,
                               FunctionCallInfo fcinfo);