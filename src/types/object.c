#include <stdlib.h>
#include <assert.h>

#include "types/object.h"
#include "utils.h"

// #include "types/string.h"
// #include "types/list.h"
// #include "types/dict.h"

// todo: push_* functions should have specified behaviour
//       but should they move values or copy them?
//       we probably need another class of functions for copies specifically
//       also we need to invalidate original moved objects, so, maybe require passing mutable pointer?
// todo: implement move_* functions for types

struct _IrisObject object_copy(const struct _IrisObject obj) {
  assert(object_is_valid(obj));
  switch (obj.kind) {
    case irisObjectKindInt:
      return (IrisObject){ .kind = irisObjectKindInt, .int_variant = obj.int_variant };
    case irisObjectKindFloat:
      return (IrisObject){ .kind = irisObjectKindFloat, .int_variant = obj.float_variant };
    case irisObjectKindString:
      return string_to_object(string_copy(obj.string_variant));
    case irisObjectKindRefCell:
      return refcell_to_object(refcell_copy(obj.refcell_variant));
    case irisObjectKindList:
      return list_to_object(list_copy(obj.list_variant));
    default:
      panic("copy behavior for object variant isn't defined");
  }
}

void object_move(IrisObject* obj) {
  assert(object_is_valid(*obj));
  switch (obj->kind) {
    case irisObjectKindInt: break;
    case irisObjectKindFloat: break;
    case irisObjectKindString:
      string_move(&obj->string_variant);
      break;
    case irisObjectKindList:
      list_move(&obj->list_variant);
      break;
    case irisObjectKindDict:
      dict_move(&obj->dict_variant);
      break;
    case irisObjectKindRefCell:
      refcell_move(&obj->refcell_variant);
      break;
    default:
      panic("move behavior for object variant isn't defined");
  }
}

size_t object_hash(const IrisObject obj) {
  assert(object_is_valid(obj));
  switch (obj.kind) {
    case irisObjectKindInt:
      return obj.int_variant;
    case irisObjectKindFloat:
      return obj.int_variant; // interpret bit layout of int, could be dangerous
    case irisObjectKindString:
      return obj.string_variant.hash;
    default:
      panic("hash behavior for object variant isn't defined");
  }
  __builtin_unreachable();
}

bool object_is_valid(const IrisObject obj) {
  if (obj.kind > N_OBJECT_KINDS) {
    return false;
  }
  switch (obj.kind) {
    case irisObjectKindNone:
      return true;
    case irisObjectKindInt:
      return true;
    case irisObjectKindFloat: // todo: check for NaN?
      return true;
    case irisObjectKindString:
      return string_is_valid(obj.string_variant);
    case irisObjectKindList:
      return list_is_valid(obj.list_variant);
    case irisObjectKindError:
      return error_is_valid(obj.error_variant);
    case irisObjectKindFunc:
      return func_is_valid(obj.func_variant);
    case irisObjectKindRefCell:
      return refcell_is_valid(obj.refcell_variant);
    default:
      panic("validity check for object variant isn't defined");
  }
  __builtin_unreachable();
}

bool object_is_none(const IrisObject obj) {
  assert(object_is_valid(obj));
  return obj.kind == irisObjectKindNone;
}

void object_destroy(IrisObject* obj) {
  assert(object_is_valid(*obj));
  switch (obj->kind) {
    case irisObjectKindNone:
      // panic("attempt to destroy None"); // todo: should it just silently escape?
      break;
    case irisObjectKindInt: break;
    case irisObjectKindFloat: break;
    case irisObjectKindString:
      string_destroy(&obj->string_variant);
      break;
    case irisObjectKindList:
      list_destroy(&obj->list_variant);
      break;
    case irisObjectKindFunc:
      func_destroy(&obj->func_variant);
      break;
    case irisObjectKindError:
      error_destroy(&obj->error_variant);
      break;
    case irisObjectKindRefCell:
      refcell_destroy(&obj->refcell_variant);
      break;
    default:
      panic("destroy behavior for object variant isn't defined");
  }
}

void object_print(const IrisObject obj, bool newline) {
  assert(object_is_valid(obj));
  switch (obj.kind) {
    case irisObjectKindNone:
      (void)fputs("None", stdout);
      if (newline) { (void)fputc('\n', stdout); }
      fflush(stdout);
      break;
    case irisObjectKindList:
      list_print_repr(obj.list_variant, newline);
      break;
    case irisObjectKindInt:
      (void)fprintf(stdout, "%d", obj.int_variant);
      if (newline) { (void)fputc('\n', stdout); }
      fflush(stdout);
      break;
    case irisObjectKindFloat:
      (void)fprintf(stdout, "%f", obj.float_variant);
      if (newline) { (void)fputc('\n', stdout); }
      fflush(stdout);
      break;
    case irisObjectKindString:
      string_print(obj.string_variant, newline);
      break;
    case irisObjectKindFunc:
      func_print_repr(obj.func_variant, newline);
      break;
    case irisObjectKindError:
      error_print_repr(obj.error_variant, newline);
      break;
    case irisObjectKindRefCell:
      refcell_print(obj.refcell_variant, newline);
      break;
    default:
      panic("printing behaviour for obj type isn't defined");
  }
}

void object_print_repr(const IrisObject obj, bool newline) {
  assert(object_is_valid(obj));
  switch (obj.kind) {
    case irisObjectKindNone:
      (void)fputs("None", stdout);
      if (newline) { (void)fputc('\n', stdout); }
      fflush(stdout);
      break;
    case irisObjectKindList:
      list_print_repr(obj.list_variant, newline);
      break;
    case irisObjectKindInt:
      (void)fprintf(stdout, "%d", obj.int_variant);
      if (newline) { (void)fputc('\n', stdout); }
      fflush(stdout);
      break;
    case irisObjectKindFloat:
      (void)fprintf(stdout, "%f", obj.float_variant);
      if (newline) { (void)fputc('\n', stdout); }
      fflush(stdout);
      break;
    case irisObjectKindString:
      string_print_repr(obj.string_variant, newline);
      break;
    case irisObjectKindFunc:
      func_print_repr(obj.func_variant, newline);
      break;
    case irisObjectKindError:
      error_print_repr(obj.error_variant, newline);
      break;
    case irisObjectKindRefCell:
      refcell_print_repr(obj.refcell_variant, newline);
      break;
    default:
      panic("printing behaviour for obj type isn't defined");
  }
}
