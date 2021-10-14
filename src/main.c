#include "iris.h"
#include "utils.h"
#include "reader.h"

void dict_test() {
  IrisDict dict_test = dict_new();
  IrisString str1 = string_from_chars("what's up?");
  IrisString str2 = string_from_chars("that's it!");
  IrisString str3 = string_from_chars("nah...");
  IrisString str4 = string_from_chars("another!...");
  // IrisString str5 = string_from_chars("even more...");
  // todo: dict_push_string?
  dict_push_object(&dict_test, str1.hash, &string_to_object(str1));
  dict_push_object(&dict_test, str2.hash, &string_to_object(str2));
  dict_push_object(&dict_test, str3.hash, &string_to_object(str3));
  // dict_push_object(&dict_test, str4.hash, &string_to_object(str4));
  // dict_push_object(&dict_test, str5.hash, &string_to_object(str5));
  print_dict(dict_test, true);
  dict_free(&dict_test);
}

int main(int argc, const char** argv) {
  dict_test();

  IrisString source = string_from_file(stdin);
  if (string_is_valid(source)) {
    IrisList sprout = nurture(source);
    print_list_debug(sprout, true);
    free_list(&sprout);
    free_string(&source);
  }
  #ifdef IRIS_COLLECT_MEMORY_METRICS
  iris_metrics_print();
  #endif
  return 0;
}
