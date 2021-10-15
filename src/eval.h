#ifndef IRIS_EVAL_H
#define IRIS_EVAL_H

#include "types/types.h"

// todo:
void enter_repl();

/*
  @brief  Evaluate list of object as it's composed from valid code
  @params scope - dictionary that contains callable and data objects, lookup of symbols is done against it
          in_repl - if true then every top-most list result will be printed, otherwise ignored
*/
void eval(const IrisList*, const IrisDict* scope, bool in_repl);

/*
  @brief Get default scope for evaluation
*/
IrisDict scope_default();

#endif
