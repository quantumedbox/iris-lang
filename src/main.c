#include "iris.h"
#include "utils.h"
#include "reader.h"

int main(int argc, const char** argv) {
  IrisString test = string_from_file(stdin);
  print_string_debug(test, true);
  free_string(test);
  return 0;
}
