#include "iris.h"
#include "iris_eval.h"
#include "iris_utils.h"

void iris_init() {
  init_error_module();
  eval_module_init();
}

void iris_deinit(void) {
  eval_module_deinit();
  deinit_error_module();
}
