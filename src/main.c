#include "iris.h"
#include "utils.h"
#include "reader.h"

int main(int argc, const char** argv) {
  IrisString test = string_from_file(stdin);
  print_string_debug(test, true);
  IrisList sprout = iris_nurture_from_string(test);
  print_list_debug(sprout, true);
  free_list(sprout);
  free_string(test);
  return 0;
}
