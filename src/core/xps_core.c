#include "xps_core.h"
#include "../network/xps_connection.h"
#include "../network/xps_listener.h"
#include "xps_loop.h"
#include "xps_pipe.h"

xps_core_t *xps_core_create() {

  xps_core_t *core = (xps_core_t *)malloc(sizeof(xps_core_t));
  if (core == NULL) {
    logger(LOG_ERROR, "xps_core_create()", "malloc() failed");
    return NULL;
  }

  xps_loop_t *loop = xps_loop_create(core);
  if (loop == NULL) {
    logger(LOG_ERROR, "xps_core_create()", "xps_loop_create() failed");
    free(core);
    return NULL;
  }

  core->loop = loop;
  core->n_null_listeners = 0;
  core->n_null_connections = 0;
  core->n_null_pipes = 0;
  vec_init(&core->pipes);
  vec_init(&core->listeners);
  vec_init(&core->connections);

  logger(LOG_DEBUG, "xps_core_create()", "core created");

  return core;
}

void xps_core_destroy(xps_core_t *core) {
  assert(core != NULL);

  for (int i = 0; i < core->connections.length; i++) {
    xps_connection_t *conn = core->connections.data[i];
    if (conn != NULL) {
      xps_connection_destroy(conn);
    }
  }
  vec_deinit(&core->connections);

  for (int i = 0; i < core->listeners.length; i++) {
    xps_listener_t *listener = core->listeners.data[i];
    if (listener != NULL) {
      xps_listener_destroy(listener);
    }
  }
  vec_deinit(&core->listeners);

  for (int i = 0; i < core->pipes.length; i++) {
    xps_pipe_t *pipe = core->pipes.data[i];
    if (pipe != NULL) {
      if (pipe->source != NULL) {
        xps_pipe_source_destroy(pipe->source);
      }
      if (pipe->sink) {
        xps_pipe_sink_destroy(pipe->sink);
      }
      xps_pipe_destroy(pipe);
    }
  }
  vec_deinit(&core->pipes);

  xps_loop_destroy(core->loop);

  free(core);

  logger(LOG_DEBUG, "xps_core_destroy()", "core destroyed");
}

void xps_core_start(xps_core_t *core) {
  assert(core != NULL);

  for (int port = 8001; port <= 8004; port++) {
    xps_listener_t *listener = xps_listener_create(core, "127.0.0.1", port);

    logger(LOG_INFO, "main()", "Server listening on port %d", port);
  }

  logger(LOG_DEBUG, "xps_core_start()", "starting core loop");

  xps_loop_run(core->loop);
}
