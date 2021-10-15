#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

#include "utils.h"

/*
  @brief  Should be used for runtime checking where cause of crush is user data, not internal state
*/
void iris_check(bool status, const char* msg) {
  if (!status) {
    panic(msg);
  }
}

void iris_check_warn(bool status, const char* msg) {
  iris_check(msg != NULL, "NULL passed as panic message");
  if (status) {
    (void)fprintf(stderr, "warning! %s\n", msg);
  }
}

noreturn void errno_panic() {
  (void)fprintf(stderr, "program panicked with %d errno value\n", errno);
  exit(1);
}

noreturn void ferror_panic(FILE* file) {
  (void)fprintf(stderr, "program panicked on file operation with %d value\n", ferror(file));
  exit(1);
}

noreturn void panic(const char* msg) {
  iris_check(msg != NULL, "NULL passed as panic message");
  (void)fprintf(stderr, "program panicked with message: \"%s\"\n", msg);
  exit(1);
}
