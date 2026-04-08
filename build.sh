gcc -g -o build/server.run \
  src/main.c \
  src/network/xps_connection.c \
  src/network/xps_listener.c \
  src/utils/xps_logger.c \
  src/utils/xps_utils.c \
  src/lib/vec/vec.c
