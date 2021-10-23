#include "iris.h"

// todo: argv list should be used for calling builtin "run"
//       this should create new instance of interpreter that runs depending on contents of argv
// todo: interpreter instances should be incapsulated and not rely on any global data
//       as all data they share is const they should be able to run concurrently

const char* help_text =
  "- Iris interpreter -\n"
  "| version -- "IRIS_VERSION"\n"
  "| compiled -- "__DATE__"\n"
  "| commands:\n"
  "|   r         : enter interactive REPL mode\n"
  "|   f <file>  : evaluate file\n"
  "|   -h --help : show this\n";

// todo: make it stack-like? in a sense that arguments are dispatched sequentially and not in fixed position manner
void run(const IrisList argument_list) {
  IrisString help_short = string_from_chars("-h");
  IrisString help_full = string_from_chars("--help");
  IrisString run_repl = string_from_chars("r");
  IrisString from_file = string_from_chars("f");
  // IrisString from_stdin = string_from_chars("i");
  if (list_has(argument_list, string_to_object(help_short)) ||
      list_has(argument_list, string_to_object(help_full))) {
    (void)fputs(help_text, stdout);
  } else if (list_has(argument_list, string_to_object(run_repl))) {
    enter_repl();
  } else if (list_has(argument_list, string_to_object(from_file))) {
    size_t file_arg_idx = list_find(argument_list, string_to_object(from_file));
    if ((file_arg_idx + 1ULL) < argument_list.len) {
      eval_file(argument_list.items[file_arg_idx + 1ULL].string_variant);
    } else {
      (void)fputs("file unspecified\n", stdout);
    }
  } else {
    (void)fputs("mode unspecified\npass -r to enter repl or -f <filename> to evaluate file\n", stdout);
  }
  string_destroy(&help_short);
  string_destroy(&help_full);
  string_destroy(&run_repl);
  string_destroy(&from_file);
}

int main(int argc, const char* argv[]) {
  iris_init();
  IrisList argv_list = list_from_chars_array(argc, argv);
  run(argv_list);
  list_destroy(&argv_list);
  iris_deinit();
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  iris_metrics_print_repr();
  #endif
  return 0;
}
