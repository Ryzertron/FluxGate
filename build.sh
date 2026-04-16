gcc -g -o build/server.run \
  src/main.c \
  src/utils/xps_logger.c \
  src/utils/xps_utils.c \
  src/utils/xps_buffer.c \
  src/lib/vec/vec.c \
  src/network/xps_connection.c \
  src/network/xps_listener.c \
  src/network/xps_upstream.c \
  src/core/xps_core.c \
  src/core/xps_loop.c \
  src/core/xps_pipe.c \
  src/core/xps_session.c \
  src/disk/xps_file.c \
  src/disk/xps_mime.c
