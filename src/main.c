#include "core/xps_core.h"
#include "xps.h"

xps_core_t *core;

void signal_handler(int signum) {
  logger(LOG_INFO, "signal_handler()", "SIGINT received, Destroying core...");
  xps_core_destroy(core);
  logger(LOG_INFO, "signal_handler()", "Core destroyed, exiting...");
  exit(OK);
}

int main() {
  signal(SIGINT, signal_handler);

  core = xps_core_create();

  if (core == NULL) {
    logger(LOG_ERROR, "main()", "Failed to create core");
    return E_FAIL;
  }
  logger(LOG_DEBUG, "main()", "Core created");

  xps_core_start(core);
}
