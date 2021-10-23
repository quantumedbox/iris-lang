#ifndef IRIS_EVAL_H
#define IRIS_EVAL_H

#include "types/types.h"

// todo: thread control
typedef struct _IrisInterpreter {
  IrisList inhereted_scopes; // immutable formed scopes
  IrisDict local_scope; // interpreter-local scope that could be modified
} IrisInterpreter;

void eval_module_init(void);
void eval_module_deinit(void);

/*
  @warn Should be called after eval_module_init()
*/
void enter_repl(void);

void eval_file(const IrisString filename);

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
