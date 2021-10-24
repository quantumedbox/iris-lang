#include <types/iris_string.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "types/iris_types.h"
#include "iris_utils.h"
#include "iris_memory.h"

#define LIST_GROW_N 4ULL
static_assert(LIST_GROW_N > 0ULL, "list item grow amount shouldn't be 0");

IrisList list_new() {
  IrisList result = {0};
  result.cap = 0;
  result.items = NULL;
  return result;
}

IrisList list_from_chars_array(int count, const char** strs) {
  IrisList result = list_new();
  for (int i = 0; i < count; i++) {
    IrisString str = string_from_chars(strs[i]);
    assert(string_is_valid(str));
    list_push_string(&result, &str);
  }
  return result;
}

IrisList list_copy(const IrisList list) {
  assert(list_is_valid(list));
  if (list_is_empty(list)) {
    return (IrisList){0};
  } else {
    IrisList result = {0};
    for (size_t i = 0; i < list.len; i++) {
      IrisObject item_copy = object_copy(list.items[i]);
      list_push_object(&result, &item_copy);
    }
    return result;
  }
}

__forceinline void list_grow(IrisList* list) {
  assert(list->len <= list->cap);
  if (list->len == list->cap) {
    list->cap += LIST_GROW_N;
    list->items = iris_resize(list->items, list->cap, IrisObject);
  }
}

void list_push_object(IrisList* list, IrisObject* obj) {
  assert(pointer_is_valid(list));
  assert(pointer_is_valid(obj));
  assert(list_is_valid(*list));
  assert(object_is_valid(*obj));
  list_grow(list);
  list->items[list->len] = *obj;
  list->len++;
  object_move(obj);
}

void list_push_int(IrisList* list, intmax_t val) {
  assert(pointer_is_valid(list));
  assert(list_is_valid(*list));
  list_grow(list);
  IrisObject item = { .kind = irisObjectKindInt, .int_variant = val };
  list->items[list->len] = item;
  list->len++;
}

void list_push_string(IrisList* list, IrisString* str) {
  assert(pointer_is_valid(list));
  assert(list_is_valid(*list));
  list_grow(list);
  IrisObject item = { .kind = irisObjectKindString, .string_variant = *str };
  list->items[list->len] = item;
  list->len++;
  string_move(str);
}

void list_push_list(IrisList* list, IrisList* val_list) {
  assert(pointer_is_valid(list));
  assert(list_is_valid(*list));
  assert(list_is_valid(*val_list));
  list_grow(list);
  IrisObject item = { .kind = irisObjectKindList, .list_variant = *val_list };
  list->items[list->len] = item;
  list->len++;
  list_move(val_list);
}

size_t list_card(const IrisList list) {
  assert(list_is_valid(list));
  return list.len;
}

bool list_is_empty(const IrisList list) {
  assert(list_is_valid(list));
  return list_card(list) == 0;
}

bool list_is_valid(const IrisList list) {
  return (((list.len == 0ULL) && (list.cap == 0ULL)) && !pointer_is_valid(list.items)) ||
    (pointer_is_valid(list.items) && (list.len <= list.cap) && (list.len != 0ULL));
}

// const struct _IrisObject*
// list_view_slice(const IrisList list,
//                 size_t* resulting_slice_len,
//                 size_t l,
//                 size_t h)
// {
//   assert(list_is_valid(list));
//   assert(pointer_is_valid(resulting_slice_len));
//   iris_check(l <= h, "low bound is greater than high one");
//   iris_check(l < list.len, "low bound is outside of list");
//   iris_check(h < list.len, "high bound is outside of list");
//   *resulting_slice_len = h - l + 1ULL;
//   return &list.items[l];
// }

IrisList list_slice(const IrisList list, size_t l, size_t h) {
  assert(list_is_valid(list));
  iris_check(l <= h, "low bound is greater than high one");
  iris_check(l < list.len, "low bound is outside of list");
  iris_check(h < list.len, "high bound is outside of list");
  IrisList result = {
    .items = iris_alloc(h - l + 1ULL, IrisObject),
    .len = h - l + 1ULL,
    .cap = (((h - l + 1ULL) / LIST_GROW_N) + 1ULL) * LIST_GROW_N, // todo: isn't really necessary i think?
  };
  size_t counter = 0ULL;
  for (size_t i = l; i <= h; i++) {
    result.items[counter++] = object_copy(list.items[i]);
  }
  return result;
}

void list_nth_set(IrisList* list, size_t idx, IrisObject* obj) {
  assert(list_is_valid(*list));
  iris_check(idx < list->len, "idx out of bounds");
  object_destroy(&list->items[idx]);
  list->items[idx] = *obj;
  object_move(obj);
}

size_t list_find(const IrisList list, const IrisObject obj) {
  assert(list_is_valid(list));
  assert(object_is_valid(obj));
  for (size_t i = 0ULL; i < list.len; i++) {
    if (object_equal(list.items[i], obj) == true) {
      return i;
    }
  }
  panic("couldn't find object in list");
}

bool list_has(const IrisList list, const IrisObject obj) {
  assert(list_is_valid(list));
  assert(object_is_valid(obj));
  for (size_t i = 0ULL; i < list.len; i++) {
    if ((obj.kind == list.items[i].kind) && (object_equal(list.items[i], obj) == true)) {
      return true;
    }
  }
  return false;
}

void list_destroy(IrisList* list) {
  assert(pointer_is_valid(list));
  assert(list_is_valid(*list));
  for (size_t i = 0ULL; i < list->len; i++) {
    object_destroy(&list->items[i]);
  }
  if (list->items != NULL) {
    iris_free(list->items);
  }
  list_move(list);
}

void list_move(IrisList* list) {
  assert(pointer_is_valid(list));
  list->items = NULL;
  list->len = 0ULL;
  list->cap = 0ULL;
}

void list_print_repr(const IrisList list, bool newline) {
  assert(list_is_valid(list));
  (void)fputc('(', stdout);
  for (size_t i = 0ULL; i < list.len; i++) {
    if (i != 0ULL) { (void)fputc(' ', stdout); }
    object_print_repr(list.items[i], false);
  }
  (void)fputc(')', stdout);
  if (newline) { (void)fputc('\n', stdout); }
  fflush(stdout);
}

void list_print_internal(const IrisList list, bool newline) {
  assert(list_is_valid(list));
  (void)fputc('(', stdout);
  for (size_t i = 0ULL; i < list.len; i++) {
    if (i != 0ULL) { (void)fputc(' ', stdout); }
    object_print_repr(list.items[i], false);
  }
  (void)fputc(')', stdout);
  if (newline) { (void)fputc('\n', stdout); }
  fflush(stdout);
}
