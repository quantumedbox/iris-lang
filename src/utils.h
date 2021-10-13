#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdio.h>
#include "iris.h"

void iris_assert(bool, const char*);

_Noreturn void errno_panic();
_Noreturn void ferror_panic(FILE*);
_Noreturn void panic(const char*);

#endif
