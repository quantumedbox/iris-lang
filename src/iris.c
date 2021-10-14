#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "iris.h"
#include "utils.h"

// todo: push_* functions should have specified behaviour
//       but should they move values or copy them?
//       we probably need another class of functions for copies specifically
//       also we need to invalidate original moved objects, so, maybe require passing mutable pointer?
// todo: implement move_* functions for types

#define LIST_PREALLOC 4U
#define STRING_PREALLOC 8U
#define DICT_PREALLOC 4U
#define DICT_GROW_FACTOR 4U // == 1 - (1 / 4) // capacity multiplies by two when factor is equal or greater than cardinality

static_assert(LIST_PREALLOC > 0U, "list preallocation shouldn't be 0");
static_assert(STRING_PREALLOC > 0U, "string preallocation shouldn't be 0");
static_assert(DICT_PREALLOC > 0U && (DICT_PREALLOC & (DICT_PREALLOC - 1U)) == 0U, "starting bucket capacity of dict should be power of 2");
static_assert(DICT_GROW_FACTOR > 1U, "invalid dict grow factor");

typedef struct {
  size_t key; // hash
  IrisObject item;
} IrisDictPair;

typedef struct _IrisDictBucket {
  // dict buckets are implemented as vectors
  IrisDictPair* pairs;
  size_t len;
} IrisDictBucket;


__forceinline void move_object(IrisObject* obj) {
  switch (obj->kind) {
    case okInt: break;
    case okString:
      obj->string_variant.data = NULL;
      obj->string_variant.len = 0ULL;
      break;
    case okList:
      obj->list_variant.items = NULL;
      obj->list_variant.len = 0ULL;
      obj->list_variant.cap = 0ULL;
      break;
    default:
      panic("move behavior for object variant isn't defined");
  }
}

void free_object(IrisObject* obj) {
  switch (obj->kind) {
    case okNone:
      panic("attempt to free None object");
      break;
    case okInt: break;
    case okString:
      free_string(&obj->string_variant);
      break;
    case okList:
      free_list(&obj->list_variant);
      break;
    default:
      panic("free behavior for object variant isn't defined");
  }
}

IrisDict dict_new() {
  IrisDict result = {
    .buckets = iris_alloc0(DICT_PREALLOC, IrisDictBucket),
    .cap = DICT_PREALLOC,
    .card = 0ULL,
  };
  return result;
}

// todo: we shouldn't have two versions of funcs, they make maintaining harder
__forceinline void dict_free_buckets(IrisDictBucket* buckets, size_t len) {
  assert(is_pointer_valid(buckets));
  for (size_t b = 0ULL; b < len; b++) {
    for (size_t p = 0ULL; p < buckets[b].len; p++) {
      assert(buckets[b].pairs != NULL);
      free_object(&buckets[b].pairs[p].item);
    }
    if (buckets[b].pairs != NULL) {
      iris_free(buckets[b].pairs);
    }
  }
  iris_free(buckets);
}

/*
  @brief  dict_free_buckets version that assumes that pairs were moved and shouldn't be freed
*/
__forceinline void dict_free_buckets_after_move(IrisDictBucket* buckets, size_t len) {
  assert(is_pointer_valid(buckets));
  for (size_t b = 0ULL; b < len; b++) {
    // for (size_t p = 0ULL; p < buckets[b].len; p++) {
    //   assert(buckets[b].pairs != NULL);
    // }
    assert((buckets[b].len > 0 && buckets[b].pairs != NULL) ||
      (buckets[b].len == 0 && buckets[b].pairs == NULL)
    );
    if (buckets[b].len > 0 && buckets[b].pairs != NULL) {
      iris_free(buckets[b].pairs);
    }
  }
  iris_free(buckets);
}

void dict_free(IrisDict* dict) {
  assert(is_pointer_valid(dict));
  dict_free_buckets(dict->buckets, dict->cap);
}

__forceinline void dict_move_pair(IrisDictBucket* buckets,
                                  size_t len,
                                  IrisDictPair pair)
{
  size_t idx = pair.key % len;
  buckets[idx].pairs = iris_resize(buckets[idx].pairs, buckets[idx].len + 1ULL, IrisDictPair);
  buckets[idx].pairs[buckets[idx].len] = pair;
  buckets[idx].len++;
}

__forceinline void dict_grow(IrisDict* dict) {
  if (dict->card >= (dict->cap - (dict->cap / DICT_GROW_FACTOR))) {
    size_t new_cap = dict->cap << 1ULL;
    IrisDictBucket* new_buckets = iris_alloc0(new_cap, IrisDictBucket);
    for (size_t b = 0; b < dict->cap; b++) {
      for (size_t p = 0; p < dict->buckets[b].len; p++) {
        dict_move_pair(new_buckets, new_cap, dict->buckets[b].pairs[p]);
      }
    }
    dict_free_buckets_after_move(dict->buckets, dict->cap);
    dict->buckets = new_buckets;
    dict->cap = new_cap;
  }
}

void dict_push_object(IrisDict* dict, size_t key, IrisObject* item) {
  dict_grow(dict);
  size_t idx = key % dict->cap;
  for (size_t i = 0; i < dict->buckets[idx].len; i++) {
    // search if key is already present, - then update the pair
    if (dict->buckets[idx].pairs[i].key == key) {
      free_object(&dict->buckets[idx].pairs[i].item);
      dict->buckets[idx].pairs[i].item = *item;
      move_object(item);
      return;
    }
  }
  // otherwise append new pair to bucket vector
  dict->buckets[idx].pairs = iris_resize(
    dict->buckets[idx].pairs,
    dict->buckets[idx].len + 1ULL,
    IrisDictPair
  );
  IrisDictPair pair = { .key = key, .item = *item };
  dict->buckets[idx].pairs[dict->buckets[idx].len] = pair;
  dict->buckets[idx].len++;
  dict->card++;
  move_object(item);
}

bool dict_has(IrisDict dict, size_t key) {
  size_t idx = key % dict.cap;
  for (size_t i = 0; i < dict.buckets[idx].len; i++) {
    if (dict.buckets[idx].pairs[i].key == key) {
      return true;
    }
  }
  return false;
}

// todo: can we check if data len is equal to len of string?
//       as we don't require null termination data by itself has no marker of end
bool string_is_valid(IrisString str) {
  if (str.len == 0ULL || !is_pointer_valid(str.data)) {
    return false;
  }
  return true;
}

/*
  @brief  djb2 hashing algorithm
          fast, but not sure about distribution
*/
__forceinline void string_hash(IrisString* str) {
  assert(is_pointer_valid(str));
  assert(string_is_valid(*str));
  size_t hash = 5381ULL;
  for (size_t i = 0; i < str->len; i++) {
    hash = ((hash << 5ULL) + hash) + str->data[i];
  }
  str->hash = hash;
}

IrisString string_from_chars(const char* chars) {
  assert(is_pointer_valid(chars));
  size_t len = strlen(chars);
  IrisString result = {
    .data = iris_alloc(len, char),
    .len = len
  };
  for (size_t i = 0ULL; i < len; i++) {
    result.data[i] = chars[i];
  }
  string_hash(&result);
  return result;
}

IrisString string_from_view(const char* low, const char* high) {
  assert(is_pointer_valid(low));
  assert(is_pointer_valid(high));
  assert(low < high);
  size_t len = (size_t)(((ptrdiff_t)high - (ptrdiff_t)low) / sizeof(char));
  IrisString result = {
    .data = iris_alloc(len, char),
    .len = len
  };
  memcpy(result.data, low, high - low);
  string_hash(&result);
  return result;
}

IrisString string_from_file(FILE* file) {
  IrisString result = { 0 };

  {
    long int restore_cursor = ftell(file);
    if (restore_cursor == -1L) { errno_panic(); }

    if (fseek(file, 0, SEEK_END) != 0) { ferror_panic(file); }

    long int end_position = ftell(file);
    if (end_position == -1L) { errno_panic(); }
    else if (end_position == 0) {
      // no characters in file stream
      return result; // zeroed string, invalid
    }
    if (fseek(file, restore_cursor, SEEK_SET) != 0) { ferror_panic(file); }
  }
  // todo: we can check length of byte stream directly, but text streams do not provide meaningful hint
  //       can we check whether it's opened in byte or text mode?
  size_t cap = STRING_PREALLOC;
  result.data = iris_alloc(STRING_PREALLOC, char);

  // todo: use fread instead
  int ch;
  while ((ch = getc(file)) != EOF) {
    assert(result.len <= cap);
    if (result.len == cap) {
      cap += STRING_PREALLOC;
      result.data = iris_resize(result.data, cap, char);
    }
    result.data[result.len++] = ch;
  }
  if (ferror(file)) { ferror_panic(file); }
  result.data = iris_resize(result.data, result.len, char);
  string_hash(&result);
  return result;
}

char nth_char(IrisString str, size_t idx) {
  #ifdef IRIS_BOUND_CHECK
  iris_assert(idx < str.len, "string element access out of bounds");
  #endif
  return str.data[idx];
}

void free_string(IrisString* str) {
  assert(is_pointer_valid(str->data));
  assert(str->len != 0ULL);
  iris_free(str->data);
  str->data = NULL;
  str->len = 0ULL;
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
  @brief  Moves variant object to list
  @warn   Passed object should no longer be used!
*/
void push_object(IrisList* list, IrisObject* obj) {
  list_grow(list);
  switch (obj->kind) {
    case okInt:
      list->items[list->len] = *obj;
      list->len++;
      break;
    default: panic("copy behavior for pushing to list not defined for type");
  }
}

/*
  @brief  Moves integer into list
*/
void push_int(IrisList* list, int val) {
  list_grow(list);
  IrisObject item = { .kind = okInt, .int_variant = val };
  list->items[list->len] = item;
  list->len++;
}

/*
  @brief  Moves string into list as string object
  @warn   Passed string should no longer be used!
*/
void push_string(IrisList* list, IrisString* str) {
  list_grow(list);
  IrisObject item = { .kind = okString, .string_variant = *str };
  list->items[list->len] = item;
  list->len++;
  str->data = NULL;
  str->len = 0ULL;
}

/*
  @brief  Moves list into list
  @warn   Passed list should no longer be used!
*/
void push_list(IrisList* list, IrisList* val_list) {
  list_grow(list);
  IrisObject item = { .kind = okList, .list_variant = *val_list };
  list->items[list->len] = item;
  list->len++;
  val_list->items = NULL;
  val_list->len = 0ULL;
  val_list->cap = 0ULL;
}

void free_list(IrisList* list) {
  assert(is_pointer_valid(list->items));
  for (size_t i = 0ULL; i < list->len; i++) {
    free_object(&list->items[i]);
  }
  iris_free(list->items);
  list->items = NULL;
  list->len = 0ULL;
  list->cap = 0ULL;
}

void print_object(IrisObject obj) {
  switch (obj.kind) {
    case okNone:
      (void)fprintf(stdout, "None!");
      break;
    case okList:
      print_list(obj.list_variant, false);
      break;
    case okInt:
      (void)fprintf(stdout, "%d", obj.int_variant);
      break;
    case okString:
      print_string(obj.string_variant, false);
      break;
    default: panic("printing behaviour for obj type isn't defined");
  }
}

void print_string(IrisString str, bool newline) {
  (void)fprintf(stdout, "\"%.*s\"", (int)str.len, str.data);
  if (newline) { fputc('\n', stdout); }
}

void print_string_debug(IrisString str, bool newline) {
  (void)fprintf(stdout, "(\"%.*s\" : len: %llu, hash: %llu)", (int)str.len, str.data, str.len, str.hash);
  if (newline) { fputc('\n', stdout); }
}

void print_list_debug(IrisList list, bool newline) {
  fputc('(', stdout);
  for (size_t i = 0ULL; i < list.len; i++) {
    if (i != 0ULL) { fputc(' ', stdout); }
    print_object(list.items[i]);
  }
  fputc(')', stdout);
  if (newline) { fputc('\n', stdout); }
}

void print_list(IrisList list, bool newline) {
  fputc('(', stdout);
  for (size_t i = 0ULL; i < list.len; i++) {
    if (i != 0ULL) { fputc(' ', stdout); }
    print_object(list.items[i]);
  }
  fputc(')', stdout);
  if (newline) { fputc('\n', stdout); }
}

void print_dict(IrisDict dict, bool newline) {
  fputc('{', stdout);
  bool put_comma = false;
  for (size_t b = 0; b < dict.cap; b++) {
    for (size_t p = 0; p < dict.buckets[b].len; p++) {
      if (!put_comma) {
        put_comma = true;
      } else {
        fputc(',', stdout);
        fputc(' ', stdout);
      }
      (void)fprintf(stdout, "%llu: ", dict.buckets[b].pairs[p].key);
      print_object(dict.buckets[b].pairs[p].item);
    }
  }
  fputc('}', stdout);
  if (newline) { fputc('\n', stdout); }
}
