#ifndef IRIS_ERROR_H
#define IRIS_ERROR_H

typedef enum {
  irisErrorNone,

  /*
    Type error
  */
  irisErrorType,

  /*
    Violation of contract between caller and callee
    Usually happens with functions and invalid arities
  */
  irisErrorContractViolation,

  IRIS_USER_ERRORS // after this user is free to define new errors
} IrisErrorType;

typedef struct _IrisError {
  IrisErrorType type;
  struct _IrisString msg;
} IrisError;

IrisError new_error(IrisErrorType);
IrisError error_from_chars(IrisErrorType, const char*);
IrisError error_from_string(IrisErrorType, struct _IrisString*);
void error_destroy(IrisError*);
void error_print_repr(const IrisError);

#endif
