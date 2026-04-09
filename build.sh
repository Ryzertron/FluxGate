gcc -g -o build/server.run \
  src/main.c \
  src/utils/xps_logger.c \
  src/utils/xps_utils.c \
  src/lib/vec/vec.c \
  src/network/xps_connection.c \
  src/network/xps_listener.c \
  src/core/xps_core.c \
  src/core/xps_loop.c
