#ifndef XPS_CORE_H
#define XPS_CORE_H

#include "../network/xps_connection.h"
#include "../network/xps_listener.h"
#include "../utils/xps_utils.h"
#include "../xps.h"
#include "xps_loop.h"

typedef struct xps_loop_s xps_loop_t;

typedef struct xps_core_s {
  xps_loop_t *loop;
  vec_void_t listeners;
  vec_void_t connections;
  u_int n_null_listeners;
  u_int n_null_connections;
} xps_core_t;

xps_core_t *xps_core_create();
void xps_core_destroy(xps_core_t *core);
void xps_core_start(xps_core_t *core);

#endif
