#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "iris.h"
#include "utils.h"

#define LIST_PREALLOC 4
#define STRING_PREALLOC 8

IrisString string_from_chars(const char* chars) {
  IrisString result = {0};
  size_t len = strlen(chars);
  result.data = iris_alloc(len, char);
  while (*chars != '\0') {
    result.data[result.len++] = *chars;
    chars++;
  }
  return result;
}

IrisString string_from_file(FILE* file) {
  IrisString result = { 0 };

  long int restore_cursor = ftell(file);
  if (restore_cursor == -1L) {
    errno_panic();
  }
  if ((fseek(file, 0, SEEK_END), ftell(file)) == 0) {
    return result; // zeroed result
  }
  fseek(file, restore_cursor, SEEK_SET);

  // todo: we can check length of byte stream directly, but text streams do not provide meaningful hint
  size_t cap = STRING_PREALLOC;
  result.data = iris_alloc(STRING_PREALLOC, char);

  int ch;
  while ((ch = getc(file)) != EOF) {
    assert(result.len <= cap);
    if (result.len == cap) {
      cap += STRING_PREALLOC;
      result.data = iris_resize(result.data, cap, char);
    }
    result.data[result.len++] = ch;
  }
  if (ferror(file)) {
    ferror_panic(file);
  }
  result.data = iris_resize(result.data, result.len, char);
  return result;
}

char nth_char(IrisString str, size_t idx) {
  #ifdef IRIS_BOUND_CHECK
  iris_assert(idx < str.len, "string element access out of bounds");
  #endif
  return str.data[idx];
}

void free_string(IrisString str) {
  // todo: should we check for double free?
  iris_free(str.data);
}

IrisList new_list() {
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
  @brief  Appends copy of object to list
*/
void push_object(IrisList* list, IrisObject obj) {
  list_grow(list);
  switch (obj.kind) {
    case okInt:
      list->items[list->len] = obj;
      list->len++;
      break;
    default: panic("copy behavior for pushing to list not defined for type");
  }
}

void push_int(IrisList* list, int val) {
  list_grow(list);
  IrisObject item = { .kind = okInt, .int_variant = val };
  list->items[list->len] = item;
  list->len++;
}

void push_symbol(IrisList* list, IrisString str) {
  list_grow(list);
  IrisObject item = { .kind = okSymbol, .symbol_variant = str };
  list->items[list->len] = item;
  list->len++;
}

void push_list(IrisList* list, IrisList val_list) {
  list_grow(list);
  IrisObject item = { .kind = okList, .list_variant = val_list };
  list->items[list->len] = item;
  list->len++;
}

void free_list(IrisList list) {
  // todo: should we check for double free?
  iris_free(list.items);
}
