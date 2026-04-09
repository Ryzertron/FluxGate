#include "xps_connection.h"
#include "../core/xps_core.h"
#include <errno.h>
#include <sys/epoll.h>

// Forward declaration of handlers
void connection_read_handler(xps_connection_t *connection);
void connection_write_handler(xps_connection_t *connection);
void connection_close_handler(xps_connection_t *connection);
void connection_loop_read_handler(xps_connection_t *connection);
void connection_loop_write_handler(xps_connection_t *connection);

// FUNCTION DEFINITIONS
xps_connection_t *xps_connection_create(xps_core_t *core, u_int sock_fd) {
  assert(sock_fd >= 0);
  assert(core != NULL);

  xps_connection_t *connection =
      (xps_connection_t *)malloc(sizeof(xps_connection_t));

  if (connection == NULL) {
    logger(LOG_ERROR, "xps_connection_create()",
           "malloc() failed for connection");
    return NULL;
  }

  xps_loop_attach(core->loop, sock_fd, EPOLLIN | EPOLLOUT | EPOLLET, connection,
                  (xps_handler_t)connection_loop_read_handler,
                  (xps_handler_t)connection_loop_write_handler,
                  (xps_handler_t)connection_close_handler);

  connection->core = core;
  connection->sock_fd = sock_fd;
  connection->listener = NULL;
  connection->remote_ip = get_remote_ip(sock_fd);
  connection->write_buff_list = xps_buffer_list_create();
  connection->read_ready = false;
  connection->write_ready = false;
  connection->send_handler = (xps_handler_t)connection_write_handler;
  connection->recv_handler = (xps_handler_t)connection_read_handler;

  vec_push(&core->connections, connection);

  logger(LOG_DEBUG, "xps_connection_create()", "Connection created");

  return connection;
}

void xps_connection_destroy(xps_connection_t *connection) {
  assert(connection != NULL);

  vec_void_t connections = connection->core->connections;

  for (int i = 0; i < connections.length; i++) {
    xps_connection_t *curr = connections.data[i];
    if (curr == connection) {
      connections.data[i] = NULL;
      break;
    }
  }

  xps_loop_detach(connection->core->loop, connection->sock_fd);

  close(connection->sock_fd);

  free(connection->remote_ip);
  free(connection);

  logger(LOG_DEBUG, "xps_connection_destroy()", "Connection destroyed");
}

void connection_read_handler(xps_connection_t *connection) {
  assert(connection != NULL);

  // read data from client using recv into a buffer of size DEFAULT_BUFFER_SIZE
  u_char *buffer = malloc(DEFAULT_BUFFER_SIZE * sizeof(u_char));
  long read_n = recv(connection->sock_fd, buffer, DEFAULT_BUFFER_SIZE - 1, 0);

  if (read_n < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      connection->read_ready = false;
      logger(LOG_DEBUG, "xps_connection_read_handler()",
             "No Data Available. Retry later.");
      return;
    } else {
      logger(LOG_ERROR, "xps_connection_read_handler()", "recv() failed");
      xps_connection_destroy(connection);
      return;
    }
  }

  if (read_n == 0) {
    logger(LOG_INFO, "xps_connection_read_handler()", "Peer closed connection");
    xps_connection_destroy(connection);
    return;
  }

  buffer[read_n] = '\0'; // Null-terminate the buffer

  printf("Received data from %s: %s\n", connection->remote_ip, buffer);

  reverse_string((char *)buffer);

  xps_buffer_t *write_buff =
      xps_buffer_create(read_n + 1, read_n + 1, (u_char *)buffer);

  xps_buffer_list_append(connection->write_buff_list, write_buff);
}

void connection_write_handler(xps_connection_t *connection) {
  assert(connection != NULL);
  if (connection->write_buff_list->len == 0) {
    logger(LOG_DEBUG, "xps_connection_write_handler()",
           "No data to send. Waiting for next write event.");
    return;
  }

  while (1) {
    xps_buffer_t *message = xps_buffer_list_read(
        connection->write_buff_list, connection->write_buff_list->len);

    if (message == NULL) {
      logger(LOG_ERROR, "xps_connection_write_handler()",
             "xps_buffer_list_read() failed");
      return;
    }

    long sent_n = send(connection->sock_fd, message->data, message->len, 0);

    if (sent_n == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        logger(LOG_DEBUG, "xps_connection_write_handler()",
               "No Buffer Space Available. Retry later.");
        connection->write_ready = false;
        return;
      }
      logger(LOG_ERROR, "xps_connection_write_handler()", "send() failed");
      xps_connection_destroy(connection);
      return;
    }
    xps_buffer_list_clear(connection->write_buff_list, sent_n);

    if (connection->write_buff_list->len <= 0) {
      return;
    }
  }
}

void connection_close_handler(xps_connection_t *connection) {
  assert(connection != NULL);

  xps_connection_destroy(connection);
  logger(LOG_INFO, "xps_connection_close_handler()", "Peer closed connection");
}

void connection_loop_read_handler(xps_connection_t *connection) {
  assert(connection != NULL);

  connection->read_ready = true;
}

void connection_loop_write_handler(xps_connection_t *connection) {
  assert(connection != NULL);

  connection->write_ready = true;
}
