#include "iris.h"

int main(int argc, const char* argv[]) {
  iris_init(argc, argv);
  enter_repl();
  iris_deinit();
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  iris_metrics_print_repr();
  #endif
  return 0;
}
