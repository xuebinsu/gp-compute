#include "catalog.h"

// We include the following .c files to use the static variables and proctions
// defined there.
#include "utils/cache/syscache.c"
#include "utils/cache/catcache.c"

void gpc_catalog_cache_insert(int cache_id, HeapTuple tuple, uint32 hash_val) {
  Index idx = HASH_INDEX(hash_val, SysCache[cache_id]->cc_nbuckets);
  CatCTup *ct =
      CatalogCacheCreateEntry(SysCache[cache_id], tuple, hash_val, idx, false);
  // increment the reference count such that the tuple would never be removed
  // from cache
  ResourceOwnerEnlargeCatCacheRefs(CurrentResourceOwner);
  ct->refcount++;
  ResourceOwnerRememberCatCacheRef(CurrentResourceOwner, &ct->tuple);
}

HeapTuple gpc_catalog_create_tuple_pg_type(TupleDesc desc) {
  Datum *values = palloc0(sizeof(Datum) * Natts_pg_type);
  bool *is_null = palloc0(sizeof(bool) * Natts_pg_type);
  NameData type_name = {0};
  values[Anum_pg_type_typname - 1] = PointerGetDatum(&type_name);
  is_null[Anum_pg_type_typdefaultbin - 1] = true;
  is_null[Anum_pg_type_typdefault - 1] = true;
  is_null[Anum_pg_type_typacl - 1] = true;

  return heap_form_tuple(desc, values, is_null);
}

HeapTuple gpc_catalog_create_tuple_pg_attr(TupleDesc desc) {
  Datum *values = palloc0(sizeof(Datum) * Natts_pg_attribute);
  bool *is_null = palloc0(sizeof(bool) * Natts_pg_attribute);
  NameData att_name = {0};
  values[Anum_pg_attribute_attname - 1] = PointerGetDatum(&att_name);
  is_null[Anum_pg_attribute_attacl - 1] = true;
  is_null[Anum_pg_attribute_attoptions - 1] = true;
  is_null[Anum_pg_attribute_attfdwoptions - 1] = true;

  return heap_form_tuple(desc, values, is_null);
}

void gpc_catalog_cache_insert_by_oid(int cache_id, HeapTuple tuple, Oid oid) {
  if (HeapTupleGetOid(tuple) == InvalidOid || oid != InvalidOid)
    HeapTupleSetOid(tuple, oid);
  uint32 hash_val =
      GetCatCacheHashValue(SysCache[cache_id], HeapTupleGetOid(tuple), 0, 0, 0);
  gpc_catalog_cache_insert(cache_id, tuple, hash_val);
}

void gpc_catalog_cache_insert_attr(HeapTuple attr_tuple) {
  Form_pg_attribute attr = (Form_pg_attribute)GETSTRUCT(attr_tuple);
  uint32 hash_val = GetCatCacheHashValue(SysCache[ATTNUM], attr->attrelid,
                                         attr->attnum, 0, 0);
  gpc_catalog_cache_insert(ATTNUM, attr_tuple, hash_val);
}

void gpc_catalog_boot_pg_type(void) {
  Relation rel = relation_open(SysCache[TYPEOID]->cc_reloid, NoLock);

  for (int i = 0; i < gpc_boot_pg_type_num_rows; i++) {
    HeapTuple type_tuple = gpc_catalog_create_tuple_pg_type(rel->rd_att);
    memcpy(GETSTRUCT(type_tuple), &(gpc_boot_pg_type[i].data),
           sizeof(FormData_pg_type));
    gpc_catalog_cache_insert_by_oid(TYPEOID, type_tuple,
                                    gpc_boot_pg_type[i].oid);
    pfree(type_tuple);
  }
}

void gpc_catalog_init(void) {
  MemoryContextInit();
  IsUnderPostmaster = false;
  SetProcessingMode(BootstrapProcessing);
  CurrentResourceOwner = ResourceOwnerCreate(NULL, "TEST");

  RelationCacheInitialize();
  InitCatalogCache();
  // init relcache for pg_class, pg_attribute, pg_proc, and pg_type
  RelationCacheInitializePhase3();
  // use SysCacheGetAttr() to init catcache to avoid touching indexes
  HeapTuple dummy_tup = palloc0(sizeof(HeapTupleData));
  dummy_tup->t_data = palloc0(sizeof(HeapTupleHeaderData));
  bool is_null = true;
  SysCacheGetAttr(PROCOID, dummy_tup, 1, &is_null);  // for pg_proc
  SysCacheGetAttr(TYPEOID, dummy_tup, 1, &is_null);  // for pg_type
  SysCacheGetAttr(RELOID, dummy_tup, 1, &is_null);   // for pg_class
  SysCacheGetAttr(ATTNUM, dummy_tup, 1, &is_null);   // for pg_attribute
  pfree(dummy_tup->t_data);
  pfree(dummy_tup);

  elog(WARNING, "BEFORE xxx INSERT TYPE TUPLES cache id = %d",
       SysCache[TYPEOID]->cc_reloid);

  gpc_catalog_boot_pg_type();

  SetProcessingMode(NormalProcessing);
}