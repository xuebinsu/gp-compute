#pragma once

#include "postgres.h"
#include "catalog/pg_authid.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_type.h"
#include "utils/fmgroids.h"

struct gpc_boot_pg_type_data { Oid oid; FormData_pg_type data; };

extern const struct gpc_boot_pg_type_data gpc_boot_pg_type[];

extern const int gpc_boot_pg_type_num_rows;
