#pragma once

#include "postgres.h"
#include "fmgr.h"

#include "utils/builtins.h"
#include "utils/memutils.h"
#include "utils/palloc.h"
#include "utils/syscache.h"
#include "utils/catcache.h"
#include "utils/relcache.h"
#include "miscadmin.h"
#include "access/htup_details.h"
#include "utils/resowner.h"
#include "utils/fmgroids.h"

#include "boot.h"

void gpc_catalog_init(void);

void gpc_catalog_cache_insert(int cache_id, HeapTuple tuple, uint32 hash_val);

HeapTuple gpc_catalog_create_tuple_pg_type(TupleDesc desc);

HeapTuple gpc_catalog_create_tuple_pg_attr(TupleDesc desc);

void gpc_catalog_cache_insert_by_oid(int cache_id, HeapTuple tuple, Oid oid);

void gpc_catalog_cache_insert_attr(HeapTuple attr_tuple);

void gpc_catalog_boot_pg_type(void);
