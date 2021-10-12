#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "utils.h"

void print_string(IrisString str, bool newline) {
  fprintf(stdout, "%.*s", (int)str.len, str.data);
  if (newline) { putc('\n', stdout); }
}

void print_string_debug(IrisString str, bool newline) {
  fprintf(stdout, "string object, len: %llu, data: \"%.*s\"", str.len, (int)str.len, str.data);
  if (newline) { putc('\n', stdout); }
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
