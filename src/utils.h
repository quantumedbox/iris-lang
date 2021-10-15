#ifndef IRIS_UTILS_H
#define IRIS_UTILS_H

#include <stdio.h>
#include <stdbool.h>

void iris_check(bool, const char*);
void warning(bool, const char*);

_Noreturn void errno_panic();
_Noreturn void ferror_panic(FILE*);
_Noreturn void panic(const char*);

#endif
