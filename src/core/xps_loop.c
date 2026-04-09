#include "xps_loop.h"

loop_event_t *loop_event_create(u_int fd, void *ptr, xps_handler_t read_cb,
                                xps_handler_t write_cb,
                                xps_handler_t close_cb) {
  assert(ptr != NULL);

  loop_event_t *event = (loop_event_t *)malloc(sizeof(loop_event_t));
  if (event == NULL) {
    logger(LOG_ERROR, "loop_event_create()", "malloc() failed");
    return NULL;
  }

  event->fd = fd;
  event->ptr = ptr;
  event->read_cb = read_cb;
  event->write_cb = write_cb;
  event->close_cb = close_cb;

  logger(LOG_DEBUG, "loop_event_create()", "event created for fd %d", fd);

  return event;
}

void loop_event_destroy(loop_event_t *event) {
  assert(event != NULL);

  free(event);

  logger(LOG_DEBUG, "loop_event_destroy()", "event destroyed for fd %d",
         event->fd);
}

xps_loop_t *xps_loop_create(xps_core_t *core) {
  assert(core != NULL);

  int ep_fd = epoll_create1(0);
  if (ep_fd < 0) {
    logger(LOG_ERROR, "xps_loop_create()", "epoll_create1() failed: %s");
    return NULL;
  }

  xps_loop_t *loop = (xps_loop_t *)malloc(sizeof(xps_loop_t));
  if (loop == NULL) {
    logger(LOG_ERROR, "xps_loop_create()", "malloc() failed");
    return NULL;
  }

  loop->core = core;
  loop->epoll_fd = ep_fd;
  loop->n_null_events = 0;
  vec_init(&loop->events);

  logger(LOG_DEBUG, "xps_loop_create()", "loop created with epoll fd %d",
         ep_fd);

  return loop;
}

void xps_loop_destroy(xps_loop_t *loop) {
  assert(loop != NULL);

  for (int i = 0; i < loop->events.length; i++) {
    loop_event_t *event = loop->events.data[i];
    if (event != NULL) {
      loop_event_destroy(event);
    }
  }

  close(loop->epoll_fd);

  logger(LOG_DEBUG, "xps_loop_destroy()", "loop destroyed with epoll fd %d",
         loop->epoll_fd);
}

int xps_loop_attach(xps_loop_t *loop, u_int fd, int event_flags, void *ptr,
                    xps_handler_t read_cb, xps_handler_t write_cb,
                    xps_handler_t close_cb) {
  assert(loop != NULL);
  assert(ptr != NULL);
  assert(read_cb != NULL);

  struct epoll_event event;
  event.data.fd = fd;
  event.events = event_flags;
  event.data.ptr = loop_event_create(fd, ptr, read_cb, write_cb, close_cb);

  if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0) {
    logger(LOG_ERROR, "xps_loop_attach()", "epoll_ctl() failed to add fd %d",
           fd);
    perror("Error message");
    return E_FAIL;
  }

  vec_push(&loop->events, event.data.ptr);

  return OK;
}

int xps_loop_detach(xps_loop_t *loop, u_int fd) {
  assert(loop != NULL);

  if (epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0) {
    logger(LOG_ERROR, "xps_loop_detach()", "epoll_ctl() failed to remove fd %d",
           fd);
    perror("Error message");
    return E_FAIL;
  }

  for (int i = 0; i < loop->events.length; i++) {
    loop_event_t *event = loop->events.data[i];
    if (event != NULL && event->fd == fd) {
      loop_event_destroy(event);
      loop->events.data[i] = NULL;
      break;
    }
  }
  loop->n_null_events++;

  logger(LOG_DEBUG, "xps_loop_detach()", "fd %d detached from loop", fd);
  return OK;
}

void xps_loop_run(xps_loop_t *loop) {
  assert(loop != NULL);

  while (1) {
    logger(LOG_DEBUG, "xps_loop_run()", "epoll_wait");
    int n_events =
        epoll_wait(loop->epoll_fd, loop->epoll_events, loop->events.length, -1);

    logger(LOG_DEBUG, "xps_loop_run()", "epoll_wait returned %d events",
           n_events);

    logger(LOG_DEBUG, "xps_loop_run()", "handling events");

    for (int i = 0; i < n_events; i++) {
      logger(LOG_DEBUG, "xps_loop_run()", "handling event no: %d/%d", i + 1,
             n_events);

      struct epoll_event curr_epoll_event = loop->epoll_events[i];
      loop_event_t *curr_loop_event = (loop_event_t *)curr_epoll_event.data.ptr;

      int curr_loop_event_idx = -1;
      for (int j = 0; j < loop->events.length; j++) {
        if (loop->events.data[j] == curr_loop_event) {
          curr_loop_event_idx = j;
          break;
        }
      }

      if (curr_loop_event_idx == -1) {
        logger(LOG_ERROR, "xps_loop_run()",
               "loop event not found. Skipping...");
        continue;
      }

#define EVENT_VALID                                                            \
  (curr_loop_event != NULL &&                                                  \
   loop->events.data[curr_loop_event_idx] == curr_loop_event)

      if (curr_epoll_event.events & (EPOLLERR | EPOLLHUP)) {
        logger(LOG_DEBUG, "xps_loop_run()", "error event for fd %d",
               curr_loop_event->fd);
        if (curr_loop_event->close_cb != NULL) {
          curr_loop_event->close_cb(curr_loop_event->ptr);
        }
      }

      if (!EVENT_VALID) {
        logger(LOG_DEBUG, "xps_loop_run()",
               "event became invalid.  Skipping...");
        continue;
      }

      if (curr_epoll_event.events & EPOLLIN) {
        logger(LOG_DEBUG, "xps_loop_run()", "read event for fd %d",
               curr_loop_event->fd);
        if (curr_loop_event->read_cb != NULL) {
          curr_loop_event->read_cb(curr_loop_event->ptr);
        }
      }

      if (!EVENT_VALID) {
        logger(LOG_DEBUG, "xps_loop_run()",
               "event became invalid.  Skipping...");
        continue;
      }

      if (curr_epoll_event.events & EPOLLOUT) {
        logger(LOG_DEBUG, "xps_loop_run()", "write event for fd %d",
               curr_loop_event->fd);
        if (curr_loop_event->write_cb != NULL) {
          curr_loop_event->write_cb(curr_loop_event->ptr);
        }
      }

#undef EVENT_VALID
    }
  }
}
