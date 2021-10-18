#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "types/types.h"
#include "memory.h"

IrisRefCell refcell_from_object(IrisObject* obj) {
  assert(object_is_valid(*obj));
  IrisRefCell result;
  result.ref = iris_alloc(1, IrisObject);
  memcpy(result.ref, obj, sizeof(IrisObject));
  result.counter = iris_alloc(1, unsigned int);
  *result.counter = 1U;
  object_move(obj);
  return result;
}

const IrisObject* refcell_view(const IrisRefCell ref) {
  assert(refcell_is_valid(ref));
  return ref.ref;
}

IrisRefCell refcell_copy(const IrisRefCell ref) {
  assert(refcell_is_valid(ref));
  (*ref.counter)++;
  return ref;
}

void refcell_destroy(IrisRefCell* ref) {
  assert(refcell_is_valid(*ref));
  assert(*ref->counter != 0U);
  (*ref->counter)--;
  if (*ref->counter == 0U) {
    object_destroy(ref->ref);
    iris_free(ref->ref);
    iris_free(ref->counter);
  }
  refcell_move(ref);
}

void refcell_move(IrisRefCell* ref) {
  ref->ref = NULL;
  ref->counter = NULL;
}

unsigned int refcell_refcount(const IrisRefCell ref) {
  assert(refcell_is_valid(ref));
  return *ref.counter;
}

bool refcell_is_valid(const IrisRefCell ref) {
  return pointer_is_valid(ref.counter) && pointer_is_valid(ref.ref); // todo: check if pointed object is valid too? also 0 counter should be invalid?
}

void refcell_print(const IrisRefCell ref, bool newline) {
  object_print(*refcell_view(ref), newline);
}

void refcell_print_repr(const IrisRefCell ref, bool newline) {
  (void)fputs("<refcell | ", stdout);
  object_print_repr(*refcell_view(ref), false);
  (void)fputc('>', stdout);
  if (newline) (void)fputc('\n', stdout);
}
