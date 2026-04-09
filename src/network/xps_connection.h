#ifndef XPS_CONNECTION_H
#define XPS_CONNECTION_H

#include "../utils/xps_utils.h"
#include "../xps.h"
#include "xps_listener.h"

typedef struct xps_connection_s {
  xps_core_t *core;
  u_int sock_fd;
  xps_listener_t *listener;
  xps_buffer_list_t *write_buff_list;
  char *remote_ip;
} xps_connection_t;

xps_connection_t *xps_connection_create(xps_core_t *core, u_int sock_fd);
void xps_connection_destroy(xps_connection_t *connection);

#endif
