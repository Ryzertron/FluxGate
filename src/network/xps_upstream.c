#include "xps_upstream.h"
#include "xps_connection.h"

xps_connection_t *xps_upstream_create(xps_core_t *core, const char *host,
                                      u_int port) {
  assert(core);
  assert(host);
  assert(port);

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    logger(LOG_ERROR, "xps_upstream_create", "Failed to create socket");
    return NULL;
  }

  struct addrinfo *servinfo = xps_getaddrinfo(host, port);
  if (servinfo == NULL) {
    logger(LOG_ERROR, "xps_upstream_create",
           "Failed to get address info for host and port");
    close(sockfd);
    return NULL;
  }
  int connect_error = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);

  if (!(connect_error == 0 || errno == EINPROGRESS)) {
    logger(LOG_ERROR, "xps_upstream_create", "connect() failed");
    freeaddrinfo(servinfo);
    close(sockfd);
    return NULL;
  }

  freeaddrinfo(servinfo);

  xps_connection_t *conn = xps_connection_create(core, sockfd);
  if (conn == NULL) {
    logger(LOG_ERROR, "xps_upstream_create",
           "Failed to create connection object");
    close(sockfd);
    return NULL;
  }
  return conn;
}
