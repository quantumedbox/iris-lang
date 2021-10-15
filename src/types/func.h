#ifndef IRIS_FUNC_H
#define IRIS_FUNC_H

#include <stddef.h>
#include <stdbool.h>

// #include "types/list.h"

/*
  @brief  Signature by which runtime evaluation funcs are hooked
          args does usually point in memory or some IrisList object
          It's responsibility of function itself to guard argument validity,
          As for caller each function is just a black box, at least on runtime
*/
typedef struct _IrisObject (*IrisFuncPrototype)(const struct _IrisObject* args, size_t arg_count);

typedef enum {
  irisFuncTypeNone,
  irisFuncTypeC,
  irisFuncTypeList,
  N_FUNC_TYPES
} IrisFuncType;

/*
  @brief  Callable object
          Might be hooked either to C or some data list that should be evaluated each time
*/
typedef struct _IrisFunc {
  IrisFuncType type;
  union {
    IrisFuncPrototype cfunc;
    struct _IrisList codedata;
  };
} IrisFunc;

// #include "types/types.h"

IrisFunc func_from_cfunc(IrisFuncPrototype);
struct _IrisObject func_call(IrisFunc, const struct _IrisObject*, size_t);
bool func_is_valid(IrisFunc);
void func_destroy(IrisFunc*);
void func_move(IrisFunc*);
void func_print(IrisFunc, bool newline);

#endif
