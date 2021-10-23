#include "iris.h"

// todo: argv list should be used for calling builtin "run"
//       this should create new instance of interpreter that runs depending on contents of argv
// todo: interpreter instances should be incapsulated and not rely on any global data
//       as all data they share is const they should be able to run concurrently

int main(int argc, const char* argv[]) {
  iris_init();
  IrisList argv_list = list_from_chars_array(argc, argv);
  IrisString run_repl = string_from_chars("r");
  IrisString from_file = string_from_chars("f");
  // IrisString from_stdin = string_from_chars("i");
  if (list_has(argv_list, string_to_object(run_repl))) {
    enter_repl();
  } else if (list_has(argv_list, string_to_object(from_file))) {
    size_t file_arg_idx = list_find(argv_list, string_to_object(from_file));
    if ((file_arg_idx + 1ULL) < argv_list.len) {
      eval_file(argv_list.items[file_arg_idx + 1ULL].string_variant);
    } else {
      (void)fputs("file unspecified\n", stdout);
    }
  } else {
    (void)fputs("mode unspecified\npass -r to enter repl or -f <filename> to evaluate file\n", stdout);
  }
  list_destroy(&argv_list);
  string_destroy(&run_repl);
  string_destroy(&from_file);
  iris_deinit();
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  iris_metrics_print_repr();
  #endif
  return 0;
}
