#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "iris.h"
#include "utils.h"

void print_string(IrisString str, bool newline) {
  fprintf(stdout, "%.*s", (int)str.len, str.data);
  if (newline) { fputc('\n', stdout); }
}

void print_string_debug(IrisString str, bool newline) {
  fprintf(stdout, "string object, len: %llu, data: \"%.*s\"", str.len, (int)str.len, str.data);
  if (newline) { fputc('\n', stdout); }
}

void print_list_debug(IrisList list, bool newline) {
  fputc('(', stdout);
  for (size_t i = 0; i < list.len; i++) {
    IrisObject item = list.items[i];
    switch (item.kind) {
      case okList:
        if (i != 0) { fputc(' ', stdout); }
        print_list_debug(item.list_variant, false);
        break;
      case okInt:
        if (i != 0) { fputc(' ', stdout); }
        fprintf(stdout, "%d", item.int_variant);
        break;
      default: panic("printing behaviour for item type isn't defined");
    }
  }
  fputc(')', stdout);
  if (newline) { fputc('\n', stdout); }
}

/*
  @brief  Assert with message
          Supposed to be used in release for edge cases, not just for debugging
*/
void iris_assert(bool status, const char* msg) {
  if (!status) {
    panic(msg);
  }
}

_Noreturn void errno_panic() {
  fprintf(stderr, "program panicked with %d errno value\n", errno);
  exit(1);
}

_Noreturn void ferror_panic(FILE* file) {
  fprintf(stderr, "program panicked on file operation with %d value\n", ferror(file));
  exit(1);
}

_Noreturn void panic(const char* msg) {
  fprintf(stderr, "program panicked with message: \"%s\"\n", msg);
  exit(1);
}
