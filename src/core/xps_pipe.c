#include "xps_pipe.h"
#include "xps_core.h"
#include <stddef.h>

xps_pipe_t *xps_pipe_create(xps_core_t *core, size_t buff_thresh,
                            xps_pipe_source_t *source, xps_pipe_sink_t *sink) {
  assert(core != NULL);
  assert(source != NULL);
  assert(sink != NULL);
  assert(buff_thresh > 0);

  xps_pipe_t *pipe = malloc(sizeof(xps_pipe_t));
  if (pipe == NULL) {
    logger(LOG_ERROR, "xps_pipe_create()", "malloc() failed for pipe");
    return NULL;
  }

  xps_buffer_list_t *buff_list = xps_buffer_list_create();
  if (buff_list == NULL) {
    logger(LOG_ERROR, "xps_pipe_create()",
           "buffer list creation failed for pipe");
    free(pipe);
    exit(E_FAIL);
  }

  pipe->core = core;
  pipe->source = NULL;
  pipe->sink = NULL;
  pipe->buff_list = buff_list;
  pipe->buff_thresh = buff_thresh;

  vec_push(&core->pipes, pipe);

  xps_pipe_attach_source(pipe, source);
  xps_pipe_attach_sink(pipe, sink);
  source->active = true;
  sink->active = true;

  logger(LOG_DEBUG, "xps_pipe_create()", "pipe created ");
  return pipe;
}

void xps_pipe_destroy(xps_pipe_t *pipe) {
  assert(pipe != NULL);

  vec_void_t pipes = pipe->core->pipes;
  for (int i = 0; i < pipes.length; i++) {
    if (pipes.data[i] == pipe) {
      pipes.data[i] = NULL;
      pipe->core->n_null_pipes++;
      break;
    }
  }

  xps_buffer_list_destroy(pipe->buff_list);
  free(pipe);
  logger(LOG_DEBUG, "xps_pipe_destroy()", "pipe destroyed");
}

bool xps_pipe_is_readable(xps_pipe_t *pipe) {
  assert(pipe != NULL);
  return pipe->buff_list->len > 0;
}

bool xps_pipe_is_writable(xps_pipe_t *pipe) {
  assert(pipe != NULL);
  return pipe->buff_list->len < pipe->buff_thresh;
}

int xps_pipe_attach_source(xps_pipe_t *pipe, xps_pipe_source_t *source) {
  assert(pipe != NULL);
  assert(source != NULL);

  if (pipe->source != NULL) {
    logger(LOG_ERROR, "xps_pipe_attach_source()",
           "pipe already has a source attached");
    return E_FAIL;
  }

  pipe->source = source;
  source->pipe = pipe;
  logger(LOG_DEBUG, "xps_pipe_attach_source()", "source attached to pipe");
  return OK;
}

int xps_pipe_detach_source(xps_pipe_t *pipe) {
  assert(pipe != NULL);

  if (pipe->source == NULL) {
    logger(LOG_ERROR, "xps_pipe_detach_source()",
           "pipe has no source attached");
    return E_FAIL;
  }

  pipe->source->pipe = NULL;
  pipe->source = NULL;
  logger(LOG_DEBUG, "xps_pipe_detach_source()", "source detached from pipe");
  return OK;
}

int xps_pipe_attach_sink(xps_pipe_t *pipe, xps_pipe_sink_t *sink) {
  assert(pipe != NULL);
  assert(sink != NULL);

  if (pipe->sink != NULL) {
    logger(LOG_ERROR, "xps_pipe_attach_sink()",
           "pipe already has a sink attached");
    return E_FAIL;
  }

  pipe->sink = sink;
  sink->pipe = pipe;
  logger(LOG_DEBUG, "xps_pipe_attach_sink()", "sink attached to pipe");
  return OK;
}

int xps_pipe_detach_sink(xps_pipe_t *pipe) {
  assert(pipe != NULL);

  if (pipe->sink == NULL) {
    logger(LOG_ERROR, "xps_pipe_detach_sink()", "pipe has no sink attached");
    return E_FAIL;
  }

  pipe->sink->pipe = NULL;
  pipe->sink = NULL;
  logger(LOG_DEBUG, "xps_pipe_detach_sink()", "sink detached from pipe");
  return OK;
}

xps_pipe_source_t *xps_pipe_source_create(void *ptr, xps_handler_t handler_cb,
                                          xps_handler_t close_cb) {
  assert(handler_cb != NULL);
  assert(close_cb != NULL);

  xps_pipe_source_t *source = malloc(sizeof(xps_pipe_source_t));

  if (source == NULL) {
    logger(LOG_ERROR, "xps_pipe_source_create()",
           "malloc() failed for pipe source");
    return NULL;
  }

  source->pipe = NULL;
  source->ready = false;
  source->active = false;
  source->handler_cb = handler_cb;
  source->close_cb = close_cb;
  source->ptr = ptr;

  logger(LOG_DEBUG, "xps_pipe_source_create()", "pipe source created");
  return source;
}

void xps_pipe_source_destroy(xps_pipe_source_t *source) {
  assert(source != NULL);

  if (source->pipe != NULL) {
    xps_pipe_detach_source(source->pipe);
  }

  free(source);
  logger(LOG_DEBUG, "xps_pipe_source_destroy()", "pipe source destroyed");
}

int xps_pipe_source_write(xps_pipe_source_t *source, xps_buffer_t *buff) {
  assert(source != NULL);
  assert(buff != NULL);

  if (source->pipe == NULL) {
    logger(LOG_ERROR, "xps_pipe_source_write()",
           "source is not attached to any pipe");
    return E_FAIL;
  }

  if (!xps_pipe_is_writable(source->pipe)) {
    logger(LOG_ERROR, "xps_pipe_source_write()",
           "pipe buffer is full, cannot write to pipe");
    return E_FAIL;
  }

  xps_buffer_t *dup_buff = xps_buffer_duplicate(buff);
  if (dup_buff == NULL) {
    logger(LOG_ERROR, "xps_pipe_source_write()",
           "buffer duplication failed for pipe source write");
    return E_FAIL;
  }

  xps_buffer_list_append(source->pipe->buff_list, dup_buff);
  return OK;
}

xps_pipe_sink_t *xps_pipe_sink_create(void *ptr, xps_handler_t handler_cb,
                                      xps_handler_t close_cb) {
  assert(handler_cb != NULL);
  assert(close_cb != NULL);

  xps_pipe_sink_t *sink = malloc(sizeof(xps_pipe_sink_t));
  if (sink == NULL) {
    logger(LOG_ERROR, "xps_pipe_sink_create()",
           "malloc() failed for pipe sink");
    return NULL;
  }

  sink->pipe = NULL;
  sink->ready = false;
  sink->active = false;
  sink->handler_cb = handler_cb;
  sink->close_cb = close_cb;
  sink->ptr = ptr;

  logger(LOG_DEBUG, "xps_pipe_sink_create()", "pipe sink created");
  return sink;
}

void xps_pipe_sink_destroy(xps_pipe_sink_t *sink) {
  assert(sink != NULL);

  if (sink->pipe != NULL) {
    xps_pipe_detach_sink(sink->pipe);
  }

  free(sink);
  logger(LOG_DEBUG, "xps_pipe_sink_destroy()", "pipe sink destroyed");
}

xps_buffer_t *xps_pipe_sink_read(xps_pipe_sink_t *sink, size_t len) {
  assert(sink != NULL);
  assert(len > 0);

  if (sink->pipe == NULL) {
    logger(LOG_ERROR, "xps_pipe_sink_read()",
           "sink is not attached to any pipe");
    return NULL;
  }

  if (len > sink->pipe->buff_list->len) {
    logger(LOG_ERROR, "xps_pipe_sink_read()",
           "requested read length exceeds available buffer length");
    return NULL;
  }

  xps_buffer_t *buff = xps_buffer_list_read(sink->pipe->buff_list, len);

  if (buff == NULL) {
    logger(LOG_ERROR, "xps_pipe_sink_read()",
           "buffer read failed for pipe sink");
    return NULL;
  }
  return buff;
}

int xps_pipe_sink_clear(xps_pipe_sink_t *sink, size_t len) {
  assert(sink != NULL);
  assert(len > 0);

  if (sink->pipe == NULL) {
    logger(LOG_ERROR, "xps_pipe_sink_clear()",
           "sink is not attached to any pipe");
    return E_FAIL;
  }

  if (len > sink->pipe->buff_list->len) {
    logger(LOG_ERROR, "xps_pipe_sink_clear()",
           "requested clear length exceeds available buffer length");
    return E_FAIL;
  }

  if (xps_buffer_list_clear(sink->pipe->buff_list, len) != OK) {
    logger(LOG_ERROR, "xps_pipe_sink_clear()",
           "buffer clear failed for pipe sink");
    return E_FAIL;
  }
  return OK;
}
