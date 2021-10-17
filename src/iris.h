#ifndef IRIS_IRIS_H
#define IRIS_IRIS_H

#include "types/types.h"
#include "reader.h"
#include "eval.h"
#include "memory.h"
#include "utils.h"

void iris_init(int argc, const char* argv[]);
void iris_deinit(void);

#endif
