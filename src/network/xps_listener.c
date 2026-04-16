#include "xps_listener.h"
#include "../core/xps_core.h"
#include "../core/xps_loop.h"
#include "../core/xps_pipe.h"
#include "xps_connection.h"

// Forward declaration of connection handler
void listener_connection_handler(xps_listener_t *listener);

xps_listener_t *xps_listener_create(xps_core_t *core, const char *host,
                                    u_int port) {
  assert(host != NULL);
  assert(is_valid_port(port));

  // Create a socket
  int sock_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (sock_fd < 0) {
    logger(LOG_ERROR, "xps_listener_create()", "socket() failed");
    perror("Error Message");
    return NULL;
  }

  // Make Address reusable
  const int enable = 1;
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) <
      0) {
    perror("setsockopt");
    close(sock_fd);
    exit(EXIT_FAILURE);
  }

  // Setup listener Address
  struct addrinfo *addr_info = xps_getaddrinfo(host, port);
  if (addr_info == NULL) {
    logger(LOG_ERROR, "xps_listener_create()", "xps_getaddrinfo failed");
    close(sock_fd);
    return NULL;
  }

  if (bind(sock_fd, addr_info->ai_addr, addr_info->ai_addrlen) < 0) {
    logger(LOG_ERROR, "xps_listener_create()", "bind() failed on %s:%u", host,
           port);
    perror("Error Message");
    freeaddrinfo(addr_info);
    close(sock_fd);
    return NULL;
  }
  freeaddrinfo(addr_info);

  if (listen(sock_fd, DEFAULT_BACKLOG) < 0) {
    logger(LOG_ERROR, "xps_listener_create()", "listen() failed on %s:%u", host,
           port);
    perror("Error Message");
    close(sock_fd);
    return NULL;
  }

  // Create xps_listener_t
  xps_listener_t *listener = (xps_listener_t *)malloc(sizeof(xps_listener_t));
  if (listener == NULL) {
    logger(LOG_ERROR, "xps_listener_create()",
           "malloc() failed for xps_listener");
    close(sock_fd);
    return NULL;
  }

  listener->core = core;
  listener->sock_fd = sock_fd;
  listener->host = strdup(host);
  listener->port = port;

  xps_loop_attach(core->loop, sock_fd, EPOLLIN | EPOLLET, listener,
                  (xps_handler_t)listener_connection_handler, NULL, NULL);

  vec_push(&core->listeners, listener);

  logger(LOG_DEBUG, "xps_listener_create()", "Listener created on %s:%u", host,
         port);

  return listener;
}

void xps_listener_destroy(xps_listener_t *listener) {

  assert(listener != NULL);

  xps_loop_detach(listener->core->loop, listener->sock_fd);

  // Set listener to NULL from global listeners vector
  vec_void_t listeners = listener->core->listeners;

  for (int i = 0; i < listeners.length; i++) {
    xps_listener_t *curr = listeners.data[i];
    if (curr == listener) {
      listeners.data[i] = NULL;
      break;
    }
  }

  close(listener->sock_fd);

  logger(LOG_DEBUG, "xps_listener_destroy()", "Listener destroyed on %s:%u",
         listener->host, listener->port);

  free(listener);
}

void listener_connection_handler(xps_listener_t *listener) {
  assert(listener != NULL);

  struct sockaddr_in conn_addr;
  socklen_t conn_addr_len = sizeof(conn_addr);

  while (1) {
    int conn_sock_fd = accept(listener->sock_fd, (struct sockaddr *)&conn_addr,
                              &conn_addr_len);

    if (conn_sock_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      logger(LOG_ERROR, "xps_listener_connection_handler()",
             "accept() failed on %s:%u", listener->host, listener->port);
      perror("Error Message");
      return;
    }

    if (make_socket_non_blocking(conn_sock_fd) < 0) {
      logger(LOG_ERROR, "xps_listener_connection_handler()",
             "make_socket_non_blocking() failed");
      close(conn_sock_fd);
      return;
    }

    xps_connection_t *connection =
        xps_connection_create(listener->core, conn_sock_fd);

    if (connection == NULL) {
      logger(LOG_ERROR, "xps_listener_connection_handler()",
             "xps_connection_create() failed");
      close(conn_sock_fd);
      return;
    }

    xps_pipe_t *pipe = xps_pipe_create(listener->core, DEFAULT_PIPE_BUFF_THRESH,
                                       connection->source, connection->sink);
    if (pipe == NULL) {
      logger(LOG_ERROR, "listener_connection_handler", "pipe creation failed");
      xps_connection_destroy(connection);
      return;
    }

    connection->listener = listener;
    logger(LOG_INFO, "xps_listener_connection_handler", "New Connection");
  }
}
