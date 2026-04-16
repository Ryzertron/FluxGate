#include "xps_file.h"
#include "../core/xps_pipe.h"
#include "xps_mime.h"

void file_source_handler(void *ptr);
void file_source_close_handler(void *ptr);

xps_file_t *xps_file_create(xps_core_t *core, const char *file_path,
                            int *error) {
  assert(core != NULL);
  assert(file_path != NULL);
  assert(error != NULL);

  *error = E_FAIL;

  xps_file_t *file = malloc(sizeof(xps_file_t));

  char *resolved_path = realpath(file_path, NULL);
  char *resolved_public = realpath("./test/public", NULL);

  if (resolved_path == NULL || resolved_public == NULL) {
    logger(LOG_ERROR, "xps_file_create()", "realpath() failed");
    free(resolved_path);
    free(resolved_public);
    free(file);
    return NULL;
  }

  size_t public_len = strlen(resolved_public);
  if (strncmp(resolved_path, resolved_public, public_len) != 0) {
    logger(LOG_WARNING, "xps_file_create()",
           "Attempt to access file outside of public directory");
    free(resolved_path);
    free(resolved_public);
    free(file);
    return NULL;
  }

  free(resolved_public);
  free(resolved_path);

  struct stat file_stat;
  if (stat(file_path, &file_stat) != 0) {
    logger(LOG_ERROR, "xps_file_create()", "stat() failed");
    *error = E_PERMISSION;
    free(file);
    return NULL;
  }

  long temp_size = file_stat.st_size;
  FILE *file_struct = fopen(file_path, "rb");
  if (file_struct == NULL) {
    if (errno == EACCES) {
      *error = E_PERMISSION;
      logger(LOG_WARNING, "xps_file_create()",
             "Permission denied while trying to open file");
    } else if (errno == ENOENT) {
      *error = E_NOTFOUND;
      logger(LOG_WARNING, "xps_file_create()",
             "File not found while trying to open file");
    }
    free(file);
    return NULL;
  }

  const char *mime_type = xps_get_mime(file_path);

  xps_pipe_source_t *source =
      xps_pipe_source_create((void *)file, (xps_handler_t)file_source_handler,
                             (xps_handler_t)file_source_close_handler);
  if (source == NULL) {
    logger(LOG_ERROR, "xps_file_create()", "Failed to create pipe source");
    fclose(file_struct);
    free(file);
    return NULL;
  }

  source->ready = true;

  file->core = core;
  file->file_path = strdup(file_path);
  file->source = source;
  file->file_struct = file_struct;
  file->size = temp_size;
  file->mime_type = mime_type;

  *error = OK;
  logger(LOG_DEBUG, "xps_file_create()", "File created successfully");
  return file;
}

void xps_file_destroy(xps_file_t *file) {
  assert(file != NULL);

  if (file->source != NULL) {
    xps_pipe_source_destroy(file->source);
  }
  free((void *)file->file_path);
  if (file->file_struct != NULL) {
    fclose(file->file_struct);
  }
  free(file);

  logger(LOG_DEBUG, "xps_file_destroy()", "File destroyed successfully");
}

void file_source_handler(void *ptr) {
  assert(ptr != NULL);

  xps_pipe_source_t *source = (xps_pipe_source_t *)ptr;
  xps_file_t *file = (xps_file_t *)source->ptr;

  xps_buffer_t *buff = xps_buffer_create(file->size, 0, NULL);
  if (buff == NULL) {
    logger(LOG_ERROR, "file_source_handler()", "Failed to create buffer");
    return;
  }

  size_t read_n = fread(buff->data, 1, buff->size, file->file_struct);
  buff->len = read_n;

  if (ferror(file->file_struct)) {
    logger(LOG_ERROR, "file_source_handler()", "Error reading from file");
    xps_file_destroy(file);
    xps_buffer_destroy(buff);
    return;
  }

  if (read_n == 0 && feof(file->file_struct)) {
    logger(LOG_DEBUG, "file_source_handler()", "End of file reached");
    xps_file_destroy(file);
    xps_buffer_destroy(buff);
    return;
  }

  int write_result = xps_pipe_source_write(source, buff);
  if (write_result != OK) {
    logger(LOG_ERROR, "file_source_handler()",
           "Failed to write to pipe source");
    xps_file_destroy(file);
    xps_buffer_destroy(buff);
    return;
  }
  xps_buffer_destroy(buff);
}

void file_source_close_handler(void *ptr) {
  assert(ptr != NULL);

  xps_pipe_source_t *source = (xps_pipe_source_t *)ptr;
  xps_file_t *file = (xps_file_t *)source->ptr;

  xps_file_destroy(file);
  logger(LOG_DEBUG, "file_source_close_handler()", "File source closed");
}
