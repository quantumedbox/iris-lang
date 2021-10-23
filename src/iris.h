#ifndef IRIS_IRIS_H
#define IRIS_IRIS_H

#include "types/types.h"
#include "reader.h"
#include "eval.h"
#include "memory.h"
#include "utils.h"

#define IRIS_VERSION "0.01:b"

void iris_init(void);
void iris_deinit(void);

#define ANSI_ESPACE "\033["
#define ANSI_ESCAPE_ERROR ANSI_ESPACE"38;41m" // red bg, white fg
#define ANSI_ESCAPE_RESET ANSI_ESPACE"0m"

#endif
