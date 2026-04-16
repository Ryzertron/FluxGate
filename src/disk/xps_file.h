#ifndef XPS_FILE_H
#define XPS_FILE_H

#include "../utils/xps_utils.h"
#include "../xps.h"

typedef struct xps_pipe_source_s xps_pipe_source_t;
typedef struct xps_core_s xps_core_t;

typedef struct xps_file_s {
  xps_core_t *core;
  const char *file_path;
  xps_pipe_source_t *source;
  FILE *file_struct;
  size_t size;
  const char *mime_type;
} xps_file_t;

xps_file_t *xps_file_create(xps_core_t *core, const char *file_path,
                            int *error);
void xps_file_destroy(xps_file_t *file);

#endif
