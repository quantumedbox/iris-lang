#ifndef IRIS_IRIS_H
#define IRIS_IRIS_H

#include "types/types.h"
#include "reader.h"
#include "eval.h"
#include "memory.h"
#include "utils.h"

#define IRIS_VERSION "0.01:a"

void iris_init(int argc, const char* argv[]);
void iris_deinit(void);

#endif
