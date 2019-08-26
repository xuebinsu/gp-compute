#pragma once

#include "postgres.h"

#include <stdlib.h>

struct gpc_vector {
  Size size;
  Size capacity;
  double scale_factor;
  void** elems;
};

struct gpc_vector* gpc_vector_create(int init_capacity, double scale_factor);

void gpc_vector_push_back(struct gpc_vector* vector, void* elem);

struct gpc_vector *gpc_vector_copy(const struct gpc_vector *src_vector);

struct gpc_vector* gpc_vector_distinct(const struct gpc_vector* vector,
                                       int (*cmp)(const void*, const void*));

void gpc_vector_free(struct gpc_vector* vector);