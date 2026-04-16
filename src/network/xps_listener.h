#ifndef XPS_LISTENER_H
#define XPS_LISTENER_H

#include "../utils/xps_utils.h"
#include "../xps.h"

typedef struct xps_core_s xps_core_t;

typedef struct xps_listener_s {
  xps_core_t *core;
  const char *host;
  u_int port;
  u_int sock_fd;
} xps_listener_t;

xps_listener_t *xps_listener_create(xps_core_t *core, const char *host,
                                    u_int port);
void xps_listener_destroy(xps_listener_t *listener);

#endif
