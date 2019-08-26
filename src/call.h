#pragma once

#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"

#include "access/htup_details.h"
#include "access/tupdesc.h"
#include "catalog/pg_attribute.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/bytea.h"
#include "utils/datum.h"
#include "utils/elog.h"
#include "utils/hsearch.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"
#include "utils/typcache.h"

#include "vector.h"
#include "status.h"
#include "serde.h"
#include "catalog.h"

#include <sys/un.h>

struct gpc_call {
  PG_FUNCTION_ARGS;
  HeapTuple proc;
  HeapTuple args;
  struct gpc_vector *types;
  struct gpc_vector *rels;
  struct gpc_vector *attrs;

  TupleDesc pg_proc_desc;
  TupleDesc args_desc;
  TupleDesc pg_type_desc;
  TupleDesc pg_class_desc;
  TupleDesc pg_attribute_desc;
  bytea *call_bytes;

  Oid ret_type;
  TupleDesc ret_desc;
  bytea *ret_bytes;

  void *lib_handle;
};

enum gpc_status gpc_parse_type(Oid type_id, int32 type_mod,
                               struct gpc_vector *types,
                               struct gpc_vector *rels,
                               struct gpc_vector *attrs,
                               TupleDesc pg_attribute_desc);

enum gpc_status gpc_call_get_proc(struct gpc_call *call, PG_FUNCTION_ARGS);

enum gpc_status gpc_call_get_args_desc(struct gpc_call *call);

enum gpc_status gpc_call_get_args_tuple(struct gpc_call *call);

enum gpc_status gpc_call_get_args_types(struct gpc_call *call);

enum gpc_status gpc_call_get_ret_type(PG_FUNCTION_ARGS, struct gpc_call *call);

int gpc_cmp_by_oid(const void *lhs, const void *rhs);

int gpc_cmp_attrs(const void *lhs, const void *rhs);

struct gpc_call *gpc_call_create(void);

void gpc_call_free(struct gpc_call *call);

void gpc_call_distinct_tuples(struct gpc_call *call);

bytea *gpc_call_send(struct gpc_call *call, struct gpc_serde_info *send_info);

struct gpc_call *gpc_call_recv(bytea *bytes,
                               struct gpc_serde_info *recv_info);

enum gpc_status gpc_call_setup(struct gpc_call *call);
