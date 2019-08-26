#include "server.h"

int main(void) {
  gpc_catalog_init();
  elog(WARNING, "GP Compute: xxx hello, server\n");

  SockAddr client_addr = {0};
  char *server_name = "gpc_server";
  int server_sock = gpc_comm_open(server_name, strlen(server_name), 0);

  bytea *call_bytes = NULL;
  int size = gpc_comm_recv(server_sock, &client_addr, &call_bytes);
  elog(WARNING, "server recv data size = %d, expected = %d\n", size,
       VARSIZE(call_bytes));

  struct gpc_serde_info *recv = gpc_serde_create_info(3);
  struct gpc_call *call = gpc_call_recv(call_bytes, recv);
  gpc_call_setup(call);
  elog(WARNING, "proc oid = %d", HeapTupleGetOid(call->proc));

  get_pkglib_path("", pkglib_path);
  elog(WARNING, "pkg lib path = %s", pkglib_path);
  // In the pg_proc tuple of the language handler:
  char *probin = "$libdir/plr";
  char *prosrc = "plr_call_handler";
  Datum ret_val = gpc_server_call_lang_handler(probin, prosrc, call);
  elog(WARNING, "ret val = %ld", ret_val);
  return 0;
}

Datum gpc_server_call_lang_handler(char *lib_path, char *handler_name,
                                   struct gpc_call *call) {
  
  PGFunction lang_handler =
      load_external_function(lib_path, handler_name, true, &(call->lib_handle));
  return lang_handler(call->fcinfo);
}