#include "iris.h"
#include "iris_eval.h"
#include "iris_utils.h"

// todo: move OS specific stuff to separate file?
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

void iris_init() {
  #ifdef _WIN32
  {
    BOOL success_out = SetConsoleOutputCP(CP_UTF8);
    BOOL success_in = SetConsoleCP(CP_UTF8);
    iris_check_warn(success_out != (BOOL)0, "cannot set terminal output to UTF8 mode");
    iris_check_warn(success_in != (BOOL)0, "cannot set terminal input to UTF8 mode");
  }
  #endif
  init_error_module();
  eval_module_init();
}

void iris_deinit(void) {
  eval_module_deinit();
  deinit_error_module();
}
