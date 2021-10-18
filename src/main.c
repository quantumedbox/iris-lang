#include "iris.h"

// todo: argv list should be used for calling builtin "run"
//       this should create new instance of interpreter that runs depending on contents of argv
// todo: interpreter instances should be incapsulated and not rely on any global data
//       as all data they share is const they should be able to run concurrently

int main(int argc, const char* argv[]) {
  iris_init(argc, argv);
  enter_repl();
  iris_deinit();
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  iris_metrics_print_repr();
  #endif
  return 0;
}
