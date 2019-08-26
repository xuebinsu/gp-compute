#include "vector.h"

#include "access/htup.h"
#include "access/htup_details.h"

struct gpc_vector *gpc_vector_create(int init_capacity, double scale_factor) {
  struct gpc_vector *vector = palloc(sizeof(struct gpc_vector));
  vector->capacity = init_capacity;
  vector->scale_factor = scale_factor;
  vector->size = 0;
  vector->elems = palloc(init_capacity * sizeof(void *));
  return vector;
}

void gpc_vector_push_back(struct gpc_vector *vector, void *elem) {
  if (vector->size == vector->capacity) {
    vector->capacity = (int)(vector->capacity * vector->scale_factor);
    vector->elems = repalloc(vector->elems, vector->capacity * sizeof(void *));
  }
  vector->elems[vector->size] = elem;
  vector->size++;
}

struct gpc_vector *gpc_vector_copy(const struct gpc_vector *src_vector) {
  struct gpc_vector *new_vector =
      gpc_vector_create(src_vector->size, src_vector->scale_factor);
  new_vector->size = src_vector->size;
  memcpy(new_vector->elems, src_vector->elems,
         src_vector->size * sizeof(void *));
  return new_vector;
}

struct gpc_vector *gpc_vector_distinct(const struct gpc_vector *vector,
                                       int (*cmp)(const void *, const void *)) {
  if (vector == NULL || vector->elems == NULL) return NULL;
  struct gpc_vector *vec_distinct = gpc_vector_copy(vector);
  if (vec_distinct->size <= 1) return vec_distinct;
  qsort(vec_distinct->elems, vec_distinct->size, sizeof(void *), cmp);

  Size j = 0;
  for (Size i = 1; i < vec_distinct->size; i++) {
    if (cmp(&(vec_distinct->elems[i]), &(vec_distinct->elems[j])) == 0)
      continue;
    j++;
    if (i != j) vec_distinct->elems[j] = vec_distinct->elems[i];
  }
  vec_distinct->size = j + 1;
  return vec_distinct;
}

void gpc_vector_free(struct gpc_vector *vector) {
  for (Size i = 0; i < vector->size; i++) {
    pfree(vector->elems[i]);
  }
  pfree(vector->elems);
  pfree(vector);
}
