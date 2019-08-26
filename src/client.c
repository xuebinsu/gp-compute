#include "client.h"
#include "call.h"
#include "comm.h"
#include "serde.h"

PG_MODULE_MAGIC;

Datum gpc_client_handle_call(PG_FUNCTION_ARGS) {
  elog(WARNING, "GP Compute: xxx hello, world");
  struct gpc_call *call = gpc_call_create();
  elog(WARNING, "gpc_call_create");
  gpc_call_get_proc(call, fcinfo);
  elog(WARNING, "gpc_call_get_proc");
  gpc_call_get_ret_type(fcinfo, call);
  elog(WARNING, "gpc_call_get_ret_type");
  gpc_call_get_args_desc(call);
  gpc_call_get_args_tuple(call);
  elog(WARNING, "gpc_call_get_args_tuple");
  gpc_call_get_args_types(call);
  elog(WARNING, "gpc_call_get_args_types");
  gpc_call_distinct_tuples(call);
  elog(WARNING, "gpc_call_distinct_tuples");
  struct gpc_serde_info *send_info = gpc_serde_create_info(1);
  elog(WARNING, "gpc_serde_create_fcinfos");
  call->call_bytes = gpc_call_send(call, send_info);
  elog(WARNING, "gpc_call_send");

  char *client_name = "gpc_client";
  int client_sock = gpc_comm_open(client_name, strlen(client_name), 0);
  elog(WARNING, "gpc_comm_open");
  SockAddr server = {0};
  char *server_name = "gpc_server";
  server.addr.ss_family = AF_UNIX;
  server.salen = sizeof(struct sockaddr_un);
  strcpy(&(((struct sockaddr_un *)&server.addr)->sun_path[1]), server_name);
  elog(WARNING, "strcpy");
  int size = gpc_comm_send(client_sock, &server, call->call_bytes);
  elog(WARNING, "gpc call bin size = %d", VARSIZE(call->call_bytes));
  elog(WARNING, "gpc_comm_send size = %d, err = %s", size, strerror(errno));
  gpc_comm_close(client_sock);
  elog(WARNING, "gpc_comm_close");
  gpc_serde_free_info(send_info);
  gpc_call_free(call);
  elog(WARNING, "GP Compute: goodbye, world");
  return 0;
}