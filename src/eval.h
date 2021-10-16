#ifndef IRIS_EVAL_H
#define IRIS_EVAL_H

#include "types/types.h"

void init_eval(void);

const IrisDict* get_standard_scope_view(void);

void enter_repl(void);

/*
  @brief  Evaluate list of object as it's composed from valid code
  @params scope - dictionary that contains callable and data objects, lookup of symbols is done against it
          in_repl - if true then every top-most list result will be printed, otherwise ignored
*/
void eval(const IrisList*, const IrisDict* scope, bool in_repl);

IrisObject eval_object(const IrisObject, const IrisDict* scope);

/*
  @brief Get default scope for evaluation
*/
IrisDict scope_default(void);

#endif
