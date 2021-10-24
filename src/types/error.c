#include <assert.h>

#include "error.h"
#include "types/types.h"
#include "utils.h"

static IrisString error_desc_table[IRIS_N_BUILTIN_ERRORS]; // todo: make it growable?

void init_error_module(void) {
  size_t desc_specified = 0ULL;
  #define init_error_module_push_desc(type, desc) {   \
    assert(type < IRIS_N_BUILTIN_ERRORS);             \
    error_desc_table[type] = string_from_chars(desc); \
    desc_specified++;                                 \
  }
  init_error_module_push_desc(irisErrorNoError,           "NoError");
  init_error_module_push_desc(irisErrorTypeError,         "TypeError");
  init_error_module_push_desc(irisErrorContractViolation, "ContractViolation");
  init_error_module_push_desc(irisErrorNameError,         "NameError");
  init_error_module_push_desc(irisErrorSyntaxError,       "SyntaxError");
  init_error_module_push_desc(irisErrorOverflowError,     "OverflowError");
  init_error_module_push_desc(irisErrorUnderflowError,    "UnderflowError");
  init_error_module_push_desc(irisErrorEncodingError,     "EncodingError");
  init_error_module_push_desc(irisErrorStackError,        "StackError");
  iris_check(desc_specified == IRIS_N_BUILTIN_ERRORS, "error description table isn't fully formed");
  #undef init_error_module_push_desc
}

void deinit_error_module(void) {
  for (size_t i = 0ULL; i < IRIS_N_BUILTIN_ERRORS; i++) {
    string_destroy(&error_desc_table[i]);
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
  assert(err.type < IRIS_N_BUILTIN_ERRORS);
  IrisString desc = error_desc_table[err.type];
  assert(string_is_valid(desc) && !string_is_empty(desc));
  string_print(desc, false);
  if (!string_is_empty(err.msg)) {
    (void)fputs(": ", stdout);
    string_print(err.msg, false);
  } else {
    (void)fputs(": --", stdout);
  }
  if (newline) (void)fputc('\n', stdout);
}
