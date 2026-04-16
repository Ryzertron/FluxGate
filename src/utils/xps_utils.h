#ifndef XPS_UTILS_H
#define XPS_UTILS_H

#include "../xps.h"
#include "xps_buffer.h"

// Structs
typedef struct xps_keyval_s {
  char *key;
  char *val;
} xps_keyval_t;

// Sockets
bool is_valid_port(u_int port);
int make_socket_non_blocking(u_int sock_fd);
struct addrinfo *xps_getaddrinfo(const char *host, u_int port);
char *get_remote_ip(u_int sock_fd);

// Other functions
void reverse_string(char *str);
void vec_filter_null(vec_void_t *v);
const char *get_file_ext(const char *file_path);

#endif
