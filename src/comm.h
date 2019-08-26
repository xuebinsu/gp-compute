#pragma once

#include "postgres.h"
#include "libpq/pqformat.h"
#include "libpq/pqcomm.h"

#include "call.h"
#include "status.h"

#include <unistd.h>

int gpc_comm_open(char *addr_name, int len, int type);

int gpc_comm_send(int sock, SockAddr *peer, bytea *bytes);

int gpc_comm_recv(int sock, SockAddr *peer, bytea **bytes_ptr);

int gpc_comm_close(int sock);
