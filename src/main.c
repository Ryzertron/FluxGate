#include "network/xps_connection.h"
#include "network/xps_listener.h"
#include "xps.h"

int epoll_fd;

struct epoll_event events[MAX_EPOLL_EVENTS];

vec_void_t listeners;
vec_void_t connections;

int main() {
  epoll_fd = xps_loop_create();

  vec_init(&listeners);
  vec_init(&connections);

  for (int port = 8001; port <= 8003; port++) {
    xps_listener_t *listener = xps_listener_create(epoll_fd, "127.0.0.1", port);

    logger(LOG_INFO, "main()", "Server listening on port %d", port);
  }

  xps_loop_run(epoll_fd);
}

int xps_loop_create() {
  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    logger(LOG_ERROR, "xps_loop_create()", "epoll_create1() failed");
    perror("Error message");
    return E_FAIL;
  }
  return epoll_fd;
}

void xps_loop_attach(int epoll_fd, int fd, int events) {
  struct epoll_event event;
  event.data.fd = fd;
  event.events = events;

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0) {
    logger(LOG_ERROR, "xps_loop_attach()", "epoll_ctl() failed to add fd");
    perror("Error message");
  }
}

void xps_loop_detach(int epoll_fd, int fd) {
  if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0) {
    logger(LOG_ERROR, "xps_loop_detach()", "epoll_ctl() failed to remove fd");
    perror("Error message");
  }
}

void xps_loop_run(int epoll_fd) {
  while (true) {
    int n = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
    if (n < 0) {
      logger(LOG_ERROR, "xps_loop_run()", "epoll_wait() failed");
      perror("Error message");
      continue;
    }

    for (int i = 0; i < n; i++) {
      int fd = events[i].data.fd;

      // Check if the fd is a listener
      bool is_listener = false;
      for (int j = 0; j < listeners.length; j++) {
        xps_listener_t *listener = listeners.data[j];
        if (listener != NULL && listener->sock_fd == fd) {
          xps_listener_connection_handler(listener);
          is_listener = true;
          break;
        }
      }

      if (!is_listener) {
        // Check if the fd is a connection
        for (int j = 0; j < connections.length; j++) {
          xps_connection_t *connection = connections.data[j];
          if (connection != NULL && connection->sock_fd == fd) {
            xps_connection_read_handler(connection);
            break;
          }
        }
      }
    }
  }
}
