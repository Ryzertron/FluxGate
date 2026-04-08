#include "xps_connection.h"

xps_connection_t *xps_connection_create(int epoll_fd, int sock_fd) {
  assert(epoll_fd >= 0);
  assert(sock_fd >= 0);

  xps_connection_t *connection =
      (xps_connection_t *)malloc(sizeof(xps_connection_t));

  if (connection == NULL) {
    logger(LOG_ERROR, "xps_connection_create()",
           "malloc() failed for connection");
    return NULL;
  }

  xps_loop_attach(epoll_fd, sock_fd, EPOLLIN);

  connection->epoll_fd = epoll_fd;
  connection->sock_fd = sock_fd;
  connection->listener = NULL;
  connection->remote_ip = get_remote_ip(sock_fd);

  vec_push(&connections, connection);

  logger(LOG_DEBUG, "xps_connection_create()", "Connection created");

  return connection;
}

void xps_connection_destroy(xps_connection_t *connection) {
  assert(connection != NULL);

  for (int i = 0; i < connections.length; i++) {
    xps_connection_t *curr = connections.data[i];
    if (curr == connection) {
      connections.data[i] = NULL;
      break;
    }
  }

  xps_loop_detach(connection->epoll_fd, connection->sock_fd);

  close(connection->sock_fd);

  free(connection->remote_ip);
  free(connection);

  logger(LOG_DEBUG, "xps_connection_destroy()", "Connection destroyed");
}

void xps_connection_read_handler(xps_connection_t *connection) {
  assert(connection != NULL);

  // read data from client using recv into a buffer of size DEFAULT_BUFFER_SIZE
  char buffer[DEFAULT_BUFFER_SIZE];
  long read_n = recv(connection->sock_fd, buffer, DEFAULT_BUFFER_SIZE - 1, 0);

  if (read_n < 0) {
    logger(LOG_ERROR, "xps_connection_read_handler()", "recv() failed");
    perror("Error Message");
    xps_connection_destroy(connection);
    return;
  }

  if (read_n == 0) {
    logger(LOG_DEBUG, "xps_connection_read_handler()",
           "Peer closed connection");
    xps_connection_destroy(connection);
    return;
  }

  buffer[read_n] = '\0'; // Null-terminate the buffer

  printf("Received data from %s: %s\n", connection->remote_ip, buffer);

  reverse_string(buffer);

  long sent_n = 0;
  long message_len = read_n; // Since we null-terminated the buffer, we can use
                             // read_n as the message length
  while (sent_n < message_len) {
    long write_n =
        send(connection->sock_fd, buffer + sent_n, message_len - sent_n, 0);
    if (write_n < 0) {
      logger(LOG_ERROR, "xps_connection_read_handler()", "send() failed");
      perror("Error Message");
      xps_connection_destroy(connection);
      return;
    }
    sent_n += write_n;
  }
}
