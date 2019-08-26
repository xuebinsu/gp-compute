#pragma once

#include "call.h"
#include "comm.h"
#include "serde.h"
#include "catalog.h"

#include "miscadmin.h"

Datum gpc_server_call_lang_handler(char *lib_path, char *handler_name,
                                   struct gpc_call *call);