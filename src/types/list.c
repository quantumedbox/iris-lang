#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "types/types.h"
#include "utils.h"
#include "memory.h"

#define LIST_PREALLOC 4U
static_assert(LIST_PREALLOC > 0U, "list preallocation shouldn't be 0");

IrisList list_new() {
  IrisList result = {0};
  result.cap = LIST_PREALLOC;
  result.items = iris_alloc(LIST_PREALLOC, IrisObject);
  return result;
}

__forceinline void list_grow(IrisList* list) {
  if (list->len == list->cap) {
    list->cap += LIST_PREALLOC;
    list->items = iris_resize(list->items, list->cap, IrisObject);
  }
}

/*
  @brief  Moves variant object to list
  @warn   Passed object should no longer be used!
*/
void list_push_object(IrisList* list, IrisObject* obj) {
  assert(pointer_is_valid(list));
  assert(list_is_valid(*list));
  list_grow(list);
  switch (obj->kind) {
    case irisObjectKindInt:
      list->items[list->len] = *obj;
      list->len++;
      list_move(&obj->list_variant);
      break;
    default: panic("copy to list behavior for pushing to list not defined for type");
  }
}

/*
  @brief  Moves integer into list
*/
void list_push_int(IrisList* list, int val) {
  assert(pointer_is_valid(list));
  assert(list_is_valid(*list));
  list_grow(list);
  IrisObject item = { .kind = irisObjectKindInt, .int_variant = val };
  list->items[list->len] = item;
  list->len++;
}

/*
  @brief  Moves string into list as string object
  @warn   Passed string should no longer be used!
*/
void list_push_string(IrisList* list, IrisString* str) {
  assert(pointer_is_valid(list));
  assert(list_is_valid(*list));
  list_grow(list);
  IrisObject item = { .kind = irisObjectKindString, .string_variant = *str };
  list->items[list->len] = item;
  list->len++;
  string_move(str);
}

/*
  @brief  Moves list into list
  @warn   Passed list should no longer be used!
*/
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

size_t list_card(IrisList list) {
  assert(list_is_valid(list));
  return list.len;
}

bool list_is_valid(IrisList list) {
  return ((list.len == list.cap == 0ULL) && !pointer_is_valid(list.items)) ||
    (pointer_is_valid(list.items) && (list.len <= list.cap));
}

const struct _IrisObject*
list_view_slice(const IrisList* list,
                size_t* resulting_slice_len,
                size_t l,
                size_t h)
{
  assert(pointer_is_valid(list));
  assert(list_is_valid(*list));
  assert(pointer_is_valid(resulting_slice_len));
  iris_check(l <= h, "low bound is greater than high one");
  iris_check(l > list->len, "low bound is outside of list");
  iris_check(h > list->len, "high bound is outside of list");
  *resulting_slice_len = h - l;
  return &list->items[l];
}

void list_destroy(IrisList* list) {
  assert(pointer_is_valid(list));
  assert(list_is_valid(*list));
  for (size_t i = 0ULL; i < list->len; i++) {
    object_destroy(&list->items[i]);
  }
  iris_free(list->items);
  list_move(list);
}

void list_move(IrisList* list) {
  assert(pointer_is_valid(list));
  list->items = NULL;
  list->len = 0ULL;
  list->cap = 0ULL;
}

void list_print(IrisList list, bool newline) {
  assert(list_is_valid(list));
  (void)fputc('(', stdout);
  for (size_t i = 0ULL; i < list.len; i++) {
    if (i != 0ULL) { (void)fputc(' ', stdout); }
    object_print(list.items[i], false);
  }
  (void)fputc(')', stdout);
  if (newline) { (void)fputc('\n', stdout); }
}

void list_print_debug(IrisList list, bool newline) {
  assert(list_is_valid(list));
  (void)fputc('(', stdout);
  for (size_t i = 0ULL; i < list.len; i++) {
    if (i != 0ULL) { (void)fputc(' ', stdout); }
    object_print(list.items[i], false);
  }
  (void)fputc(')', stdout);
  if (newline) { (void)fputc('\n', stdout); }
}
