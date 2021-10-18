#include <assert.h>

#include "types/types.h"
#include "memory.h"
#include "utils.h"

IrisFunc func_from_cfunc(IrisFuncPrototype cfunc) {
  assert(pointer_is_valid(cfunc));
  IrisFunc result = { .type = irisFuncTypeC, .cfunc = cfunc /*, .is_macro = false*/ };
  return result;
}

IrisFunc func_macro_from_cfunc(IrisFuncPrototype cfunc) {
  assert(pointer_is_valid(cfunc));
  IrisFunc result = { .type = irisFuncTypeC, .cfunc = cfunc, .is_macro = true };
  return result;
}

IrisFunc func_copy(const IrisFunc func) {
  assert(func_is_valid(func));
  switch (func.type) {
    case irisFuncTypeC:
      return func;
    default:
      panic("undefined behavior for copying function type");
  }
}

IrisObject func_call(const IrisFunc func, const IrisObject* args, size_t arg_count) {
  assert(func_is_valid(func));
  assert(((arg_count > 0ULL) && pointer_is_valid(args)) || (arg_count == 0ULL /*&& !pointer_is_valid(args)*/));
  IrisObject result = {0};
  switch (func.type) {
    case irisFuncTypeC:
      result = func.cfunc(args, arg_count);
      break;
    default:
      panic("unsupported function type"); // unreachable, func_is_valid should cover such cases
  }
  assert(object_is_valid(result));
  return result;
}

bool func_is_valid(const IrisFunc func) {
  switch (func.type) {
    case irisFuncTypeC:
      return pointer_is_valid(func.cfunc);
    case irisFuncTypeNone:
      return false;
    default:
      panic("unsupported function type");
  }
}

void func_destroy(IrisFunc* func) {
  assert(func_is_valid(*func));
  if ((IrisObjectKind)func->type == irisObjectKindList) {
    list_destroy(&func->codedata);
  }
  func_move(func);
}

void func_move(IrisFunc* func) {
  func->type = irisFuncTypeNone;
  // func->cfunc = NULL;
}

// todo: provide function 'database' for retrieving special data
//       docstring also should be stored outside
void func_print_repr(const IrisFunc func, bool newline) {
  assert(func_is_valid(func));
  (void)fprintf(stdout, "<callable>");
  if (newline) { (void)fputc('\n', stdout); }
}

void func_print_internal(const IrisFunc func, bool newline) {
  assert(func_is_valid(func));
  switch (func.type) {
    case irisFuncTypeC:
      (void)fprintf(stdout, "<callable | cfunc: %p>", func.cfunc);
      if (newline) { (void)fputc('\n', stdout); }
      break;
    case irisFuncTypeList:
      (void)fprintf(stdout, "<callable | codedata: ");
      list_print_repr(func.codedata, false);
      (void)fputc('>', stdout);
      if (newline) { (void)fputc('\n', stdout); }
      break;
    default:
      panic("internal printing for function variant unspecified");
  }
}
