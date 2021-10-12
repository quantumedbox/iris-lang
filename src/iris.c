#include <string.h>
#include <stdlib.h>
#include "iris.h"
#include "utils.h"

#define LIST_PREALLOC 4
#define STRING_PREALLOC 8

IrisString string_from_chars(const char* chars) {
  IrisString result = {0};
  size_t len = strlen(chars);
  result.data = (char*)malloc(sizeof(char) * len);
  while (*chars != '\0') {
    result.data[result.len++] = *chars;
    chars++;
  }
  return result;
}

IrisString string_from_file(FILE* file) {
  // todo: we can check length of byte stream directly, but text streams do not provide meaningful data
  size_t buff_cap = STRING_PREALLOC;
  size_t len = 0;
  char* buff = (char*)malloc(STRING_PREALLOC * sizeof(char));

  int ch;
  while ((ch = getc(file)) != EOF) {
    if (len == buff_cap) {
      buff_cap += STRING_PREALLOC;
      buff = (char*)realloc(buff, buff_cap * sizeof(char));
    }
    buff[len++] = ch;
  }

  if (ferror(file)) { ferror_panic(file); }

  buff = (char*)realloc(buff, len * sizeof(char)); // truncation for optimal fits

  IrisString result = { .data = buff, .len = len };
  return result;
}

void free_string(IrisString str) {
  // todo: should we check for double free?
  free(str.data);
}

IrisList new_list() {
  IrisList result = {0};
  result.cap = LIST_PREALLOC;
  result.items = (IrisObject*)malloc(LIST_PREALLOC * sizeof(IrisObject));
  return result;
}

__forceinline void list_grow(IrisList* list) {
  if (list->len == list->cap) {
    list->cap += LIST_PREALLOC;
    list->items = (IrisObject*)realloc(list->items, list->cap * sizeof(IrisObject));
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
  free(list.items);
}
