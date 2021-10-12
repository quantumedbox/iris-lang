#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include "iris.h"

void print_string(IrisString, bool newline);
void print_string_debug(IrisString, bool newline);

_Noreturn void errno_panic();
_Noreturn void ferror_panic(FILE*);
_Noreturn void panic(const char*);

#endif
