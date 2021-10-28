#include <assert.h>

#include "iris.h"
#include "iris_misc.h"

// todo: interpreter instances should be incapsulated and not rely on any global data
//       as all data they share is const they should be able to run concurrently

const char* help_text =
  "- Iris Interpreter -\n"
  "| version -- "IRIS_VERSION"\n"
  "| compiled -- "__DATE__"\n"
  "| commands:\n"
  "|   r         : enter interactive REPL mode\n"
  "|   f <file>  : evaluate file\n"
  "|   -h --help : show this\n";

/*
  @brief  Executes argument list
*/
void run_with_args(const IrisList argument_list) {
  assert(list_is_valid(argument_list));
  if (argument_list.len == 1ULL) {
    (void)fputs("mode unspecified\npass 'r' to enter repl or 'f <filename>' to evaluate file\n", stdout);
    (void)fputs(help_text, stdout);
  }
  for (size_t i = 1ULL; i < argument_list.len; i++) {
    IrisObject item = argument_list.items[i];
    if (item.kind != irisObjectKindString) {
      continue;
    }
    if (string_compare_chars(item.string_variant, "-h") ||
        string_compare_chars(item.string_variant, "--help")) {
      (void)fputs(help_text, stdout);
    } else if (string_compare_chars(item.string_variant, "r")) {
      enter_repl();
    } else if (string_compare_chars(item.string_variant, "f")) {
      if (i == argument_list.len - 1ULL) {
        panic("filename unspecified");
      }
      IrisObject file = argument_list.items[i + 1ULL];
      if (file.kind != irisObjectKindString) {
        panic("filename should be string");
      }
      eval_file(file.string_variant);
      i++;
    } else {
      panic("unknown option");
    }
  }
}

int main(int argc, const char* argv[]) {
  iris_init();
  IrisList argv_list = list_from_chars_array(argc, argv);
  run_with_args(argv_list);
  list_destroy(&argv_list);
  iris_deinit();
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  iris_metrics_print_repr();
  #endif
  return 0;
}
