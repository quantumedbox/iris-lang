#include "types/types.h"
#include "utils.h"
#include "reader.h"
#include "memory.h"
#include "eval.h"

int main(int argc, const char** argv) {
  enter_repl();
  // IrisString source = string_from_file(stdin);
  // if (!string_is_empty(source)) {
  //   IrisDict scope = scope_default();
  //   IrisList sprout = nurture(source);
  //   eval(&sprout, &scope, false);
  //   list_destroy(&sprout);
  //   dict_destroy(&scope);
  //   string_destroy(&source);
  // }
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  iris_metrics_print_repr();
  #endif
  return 0;
}
