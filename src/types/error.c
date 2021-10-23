#include <assert.h>

#include "error.h"
#include "types/types.h"
#include "utils.h"

// todo: make it array instead as type do correlate with indexes
static IrisDict error_desc = {0};
static unsigned int error_enum = IRIS_USER_ERRORS;

void init_error_module(void) {
  #define init_error_module_push_desc(type, desc) {           \
    IrisString str = string_from_chars(desc);                 \
    dict_push_string(&error_desc, int_to_object(type), &str); \
  }
  error_desc = dict_new();
  init_error_module_push_desc(irisErrorNoError,           "NoError");
  init_error_module_push_desc(irisErrorTypeError,         "TypeError");
  init_error_module_push_desc(irisErrorContractViolation, "ContractViolation");
  init_error_module_push_desc(irisErrorNameError,         "NameError");
  init_error_module_push_desc(irisErrorSyntaxError,       "SyntaxError");
  init_error_module_push_desc(irisErrorOverflowError,     "OverflowError");
  init_error_module_push_desc(irisErrorUnderflowError,    "UnderflowError");
  init_error_module_push_desc(irisErrorEncodingError,     "EncodingError");
  init_error_module_push_desc(irisErrorStackError,        "StackError");
  init_error_module_push_desc(irisErrorUserError,         "UserError");
  iris_check(error_desc.card == IRIS_USER_ERRORS, "error description missed");
}

void deinit_error_module(void) {
  if (dict_is_valid(error_desc)) {
    dict_destroy(&error_desc);
  } else {
    panic("error description dictionary is ill-formed");
  }
}

IrisError error_new(IrisErrorType type) {
  IrisError result = { .type = type, .msg = (IrisString){0} };
  return result;
}

IrisError error_from_chars(IrisErrorType type, const char* chars) {
  IrisString msg = string_from_chars(chars);
  IrisError result = { .type = type, .msg = msg };
  return result;
}

IrisError error_from_string(IrisErrorType type, IrisString* str) {
  IrisError result = { .type = type, .msg = *str };
  string_move(str);
  return result;
}

IrisError error_copy(const IrisError err) {
  IrisError result = { .type = err.type };
  if (!string_is_empty(err.msg)) {
    result.msg = string_copy(err.msg);
  }
  return result;
}

bool error_is_valid(const IrisError err) {
  (void)err;
  return true;
}

void error_destroy(IrisError* err) {
  string_destroy(&err->msg);
  error_move(err);
}

void error_move(IrisError* err) {
  err->type = irisErrorNoError;
  err->msg = (IrisString){0};
}

void error_print_repr(const IrisError err, bool newline) {
  assert((err.type < error_enum) && (err.type >= irisErrorNoError));
  const IrisObject* desc = dict_get_view(error_desc, int_to_object(err.type)); // will fail if errtype isn't implemented
  string_print(desc->string_variant, false);
  if (!string_is_empty(err.msg)) {
    (void)fputs(": ", stdout);
    string_print(err.msg, false);
  } else {
    (void)fputs(": -", stdout);
  }
  if (newline) (void)fputc('\n', stdout);
}
