#ifndef IRIS_ERROR_H
#define IRIS_ERROR_H

#include <stdbool.h>

#include "types/string.h"

typedef enum {
  irisErrorNoError,

  /*
    Type error
  */
  irisErrorTypeError,

  /*
    Violation of contract between caller and callee
    Usually happens with functions and invalid arities
  */
  irisErrorContractViolation,

  /*
    Says that something went wrong on scope resolution
  */
  irisErrorNameError,

  /*
    Unspecified error that might be used for signaling error messages of API
  */
  irisErrorUserError,

  IRIS_USER_ERRORS // after this user is free to define new errors
} IrisErrorType;

typedef struct _IrisError {
  IrisErrorType type;
  struct _IrisString msg;
} IrisError;

void init_error_module(void);
IrisError error_new(IrisErrorType);
IrisError error_from_chars(IrisErrorType, const char*);
IrisError error_from_string(IrisErrorType, struct _IrisString*);
bool error_is_valid(const IrisError);
void error_destroy(IrisError*);
void error_move(IrisError*);
void error_print_repr(const IrisError, bool newline);

#define error_to_object(err) (struct _IrisObject){ .kind = irisObjectKindError, .error_variant = (err) }

#define catch_error(err)                    \
  do {                                      \
    if (err.kind == irisObjectKindError) {  \
      return err;                           \
    }                                       \
  } while (0)                               \

#endif
