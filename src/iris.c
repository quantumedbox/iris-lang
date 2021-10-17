#include "iris.h"
#include "eval.h"
#include "utils.h"

void iris_init(int argc, const char* argv[]) {
  init_error_module();
  eval_module_init(argc, argv);
}

void iris_deinit(void) {
  eval_module_deinit();
  deinit_error_module();
}
