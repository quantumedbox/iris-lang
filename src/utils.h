#ifndef IRIS_UTILS_H
#define IRIS_UTILS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdnoreturn.h>

#ifndef IRIS_NO_CHECKS
void iris_check(bool, const char*);
void iris_check_warn(bool, const char*);
#else
#define iris_check(_test, _msg) //
#define iris_check_warn(_test, _msg) //
#endif

noreturn void errno_panic(void);
noreturn void ferror_panic(FILE*);
noreturn void panic(const char*);
void warning(const char*);

#endif
