#ifndef IRIS_UTILS_H
#define IRIS_UTILS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdnoreturn.h>

void iris_check(bool, const char*);
void iris_check_warn(bool, const char*);

noreturn void errno_panic(void);
noreturn void ferror_panic(FILE*);
noreturn void panic(const char*);
void warning(const char*);

#endif
