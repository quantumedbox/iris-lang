#include "types/types.h"
#include "utils.h"
#include "reader.h"
#include "memory.h"

void dict_test() {
  IrisDict dict_test = dict_new();
  IrisDict dict_test2 = dict_new();

  IrisString str1 = string_from_chars("what's up?");
  IrisString str2 = string_from_chars("that's it!");
  IrisString str3 = string_from_chars("nah...");
  IrisString str4 = string_from_chars("another!...");
  IrisString str5 = string_from_chars("even more...");
  // todo: dict_push_string?
  dict_push_string(&dict_test2, &str1);
  string_print(str1, true);
  dict_push_string(&dict_test, &str1);
  dict_push_string(&dict_test, &str2);
  dict_push_string(&dict_test, &str3);
  dict_push_string(&dict_test, &str4);
  dict_push_string(&dict_test, &str5);
  dict_print(dict_test, true);

  dict_destroy(&dict_test);
}

int main(int argc, const char** argv) {
  dict_test();

  // IrisString source = string_from_file(stdin);
  // if (!string_is_empty(source)) {
  //   IrisList sprout = nurture(source);
  //   print_list_debug(sprout, true);
  //   free_list(&sprout);
  //   free_string(&source);
  // }
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  iris_metrics_print();
  #endif
  return 0;
}
