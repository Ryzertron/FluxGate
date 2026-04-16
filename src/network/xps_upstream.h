#ifndef XPS_UPSTREAM_H
#define XPS_UPSTREAM_H

#include "../utils/xps_utils.h"
#include "../xps.h"

typedef struct xps_connection_s xps_connection_t;
typedef struct xps_core_s xps_core_t;

xps_connection_t *xps_upstream_create(xps_core_t *core, const char *host,
                                      u_int port);

#endif
