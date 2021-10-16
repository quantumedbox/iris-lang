#include <string.h>
#include <stdio.h>
#include <assert.h>

// todo: it's not very good that header of this file is colliding with <string.h>
//       we're using it by "types/string.h" for now, but it might be problematic in the future

// todo: it might be better to shift length for it to be that 0 is always erroneous and actual 0 length strings are equal to 1
//       then string with 4 characters has 5 virtual length

// todo: when reading from file stream there's no way to singal status of reading to caller,
//       we should consider some way to do so

// todo: should they be cached on creation or only on first hash request?

// todo: implementation of 0 length strings is awful, should redo it

#include "types/types.h"
// #include "types/string.h"
#include "memory.h"
#include "utils.h"

#define STRING_PREALLOC 8U
static_assert(STRING_PREALLOC > 0U, "string preallocation shouldn't be 0");

bool string_is_valid(const IrisString str) {
  return ((str.len == 0ULL && !pointer_is_valid(str.data)) ||
    (str.len > 0ULL && pointer_is_valid(str.data)));
}

bool string_is_empty(const IrisString str) {
  assert(string_is_valid(str));
  return str.len == 0ULL;
}

/*
  @brief  djb2 hashing algorithm
          fast, but not sure about distribution
          main problem is that it's extremely likely to have collisions with integers
*/
void string_hash(IrisString* str) {
  assert(pointer_is_valid(str));
  assert(string_is_valid(*str));
  size_t hash = 5381ULL;
  for (size_t i = 0; i < str->len; i++) {
    hash = ((hash << 5ULL) + hash) + str->data[i];
  }
  str->hash = hash;
}

IrisString string_from_chars(const char* chars) {
  // assert(pointer_is_valid(chars));
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
  assert(pointer_is_valid(low));
  assert(pointer_is_valid(high));
  assert(low <= high);
  size_t len = (size_t)(((ptrdiff_t)high - (ptrdiff_t)low) / sizeof(char));
  IrisString result = {
    .data = iris_alloc(len, char),
    .len = len
  };
  memcpy(result.data, low, high - low);
  string_hash(&result);
  return result;
}

IrisString string_from_file_line(FILE* file) {
  IrisString result = {0};
  size_t cap = 0ULL;

  int ch;
  while ((ch = getc(file)) != '\n' && ch != EOF) {
    if (cap <= result.len) {
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

IrisString string_from_file(FILE* file) {
  IrisString result = {0};

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

bool string_compare(const IrisString x, const IrisString y) {
  assert(string_is_valid(x));
  assert(string_is_valid(y));
  #ifndef IRIS_SECURE
  return x.hash == y.hash;
  #else
  if (x.len != y.len) {
    return false;
  }
  for (size_t i = 0ULL; i < x.len; i++) {
    if (x.data[i] != y.data[y]) {
      return false;
    }
  }
  return true;
  #endif
}

size_t string_card(const IrisString str) {
  assert(string_is_valid(str));
  return str.len;
}

char string_nth(const IrisString str, size_t idx) {
  assert(string_is_valid(str));
  iris_check(idx < str.len, "given idx isn't within string boundaries");
  return str.data[idx];
}

void string_destroy(IrisString* str) {
  // currently we set len to inappropriate value for NULL data
  // it's kinda hacky, but works for now 
  iris_check(string_is_valid(*str), "attempt to double free on string");
  // it's assumed that check above will stop str->len == 1ULL && str->data == NULL case
  if (str->len != 0ULL) {
    iris_free(str->data);
  }
  string_move(str);
}

void string_move(IrisString* str) {
  str->data = NULL;
  str->len = 1ULL;
}

void string_print_repr(const IrisString str, bool newline) {
  (void)fprintf(stdout, "%.*s", (int)str.len, str.data);
  if (newline) { (void)fputc('\n', stdout); }
  fflush(stdout);
}

void string_print_internal(const IrisString str, bool newline) {
  (void)fprintf(stdout, "<string | bytes: \"%.*s\" : len: %llu, hash: %llu)", (int)str.len, str.data, str.len, str.hash);
  if (newline) { (void)fputc('\n', stdout); }
  fflush(stdout);
}
