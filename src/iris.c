#include "locale.h"

#include "iris.h"
#include "eval.h"
#include "utils.h"

void iris_init() {
  setlocale(LC_ALL, ".utf8"); // todo: is it operates on process? could be problems with that, probably should initialize interpreter threads locally
  init_error_module();
  eval_module_init();
}

void iris_deinit(void) {
  eval_module_deinit();
  deinit_error_module();
}
