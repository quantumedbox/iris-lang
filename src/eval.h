#ifndef IRIS_EVAL_H
#define IRIS_EVAL_H

#include "types/types.h"

void eval_module_init(int argc, const char* argv[]);
void eval_module_deinit(void);

/*
  @warn Should be called after eval_module_init()
*/
void enter_repl(void);

/*
  @warn Should be called after eval_module_init()
*/
const IrisDict* get_standard_scope_view(void);

/*
  @brief  Evaluate list of object as it's composed from valid code
          Returns result of evaluation of the last element
  @params scope - dictionary that contains callable and data objects, lookup of symbols is done against it
          in_repl - if true then every top-most list result will be printed, otherwise ignored
*/
IrisObject eval_codelist(const IrisList);
IrisObject eval_object(const IrisObject);

#endif
