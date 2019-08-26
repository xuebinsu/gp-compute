#include "comm.h"

int gpc_comm_open(char *addr_name, int len, int type) {
  int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;
  strncpy(&(addr.sun_path[1]), addr_name, len);
  bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));
  return sock;
}

int gpc_comm_send(int sock, SockAddr *peer, bytea *bytes) {
  return sendto(sock, bytes, VARSIZE(bytes), 0,
                (struct sockaddr *)&(peer->addr), peer->salen);
}

int gpc_comm_recv(int sock, SockAddr *peer, bytea **bytes_ptr) {
  if (*bytes_ptr == NULL) {
    int ret = recvfrom(sock, bytes_ptr, VARHDRSZ, MSG_PEEK | MSG_TRUNC,
                       (struct sockaddr *)&(peer->addr), &peer->salen);
    if (ret < VARHDRSZ) return ret;
    int size = VARSIZE(bytes_ptr);
    *bytes_ptr = palloc(size);
    SET_VARSIZE(*bytes_ptr, size);
  }
  return recvfrom(sock, *bytes_ptr, VARSIZE(bytes_ptr), 0,
                  (struct sockaddr *)&(peer->addr), &peer->salen);
}

int gpc_comm_close(int sock) { return close(sock); }