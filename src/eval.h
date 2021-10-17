#ifndef IRIS_EVAL_H
#define IRIS_EVAL_H

#include "types/types.h"

// todo: define rules about ownership of objects in evaluation

void eval_module_init(int argc, const char* argv[]);
void eval_module_deinit(void);

/*
  @warn Should be called after eval_module_init()
*/
const IrisDict* get_standard_scope_view(void);

/*
  @warn Should be called after eval_module_init()
*/
void enter_repl(void);

/*
  @brief  Evaluate list of object as it's composed from valid code
  @params scope - dictionary that contains callable and data objects, lookup of symbols is done against it
          in_repl - if true then every top-most list result will be printed, otherwise ignored
*/
void eval(const IrisList*, const IrisDict* scope, bool in_repl);

IrisObject eval_object(const IrisObject, const IrisDict* scope);

#endif
