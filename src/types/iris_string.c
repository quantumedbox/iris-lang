#include <string.h>
#include <stdio.h>
#include <assert.h>

// todo: it's not very good that header of this file is colliding with <types/iris_string.h>
//       we're using it by "types/iris_string.h" for now, but it might be problematic in the future

// todo: when reading from file stream there's no way to singal status of reading to caller,
//       we should consider some way to do so

// todo: should they be cached on creation or only on first hash request?

// todo: access of runes / individual characters
// todo: search in utf8 encoded string might be done from the end if end is closer to sought-for rune

#include "types/iris_types.h"
#include "iris_memory.h"
#include "iris_utils.h"
#include "iris_utf8.h"

#define STRING_PREALLOC 8U
static_assert(STRING_PREALLOC > 0U, "string preallocation shouldn't be 0");

bool string_is_valid(const IrisString str) {
  return (str.len == 0ULL && !pointer_is_valid(str.data)) ||
    (str.len != 0ULL && pointer_is_valid(str.data));
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

IrisString string_copy(const IrisString str) {
  assert(string_is_valid(str));
  IrisString result = {
    .data = iris_alloc(str.len, char),
    .len = str.len,
    .hash = str.hash
  };
  memcpy(result.data, str.data, str.len * sizeof(char));
  return result;
}

IrisString string_from_chars(const char* chars) {
  // assert(pointer_is_valid(chars));
  size_t len = strlen(chars);
  IrisString result = {
    .data = iris_alloc(len, char),
    .len = len
  };
  memcpy(result.data, chars, len);
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

// todo: ignore BOM?
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
    if (x.data[i] != y.data[i]) {
      return false;
    }
  }
  return true;
  #endif
}

bool string_compare_chars(const IrisString str, const char* chars) {
  size_t len = strlen(chars);
  if (len != str.len) {
    return false;
  }
  for (size_t i = 0ULL; i < len; i++) {
    if (str.data[i] != chars[i]) {
      return false;
    }
  }
  return true;
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

bool string_equal(const IrisString x, const IrisString y) {
  assert(string_is_valid(x));
  assert(string_is_valid(y));
  return x.hash == y.hash;
}

void string_destroy(IrisString* str) {
  // currently we set len to inappropriate value for NULL data
  // it's kinda hacky, but works for now 
  assert(string_is_valid(*str));
  // it's assumed that check above will stop str->len == 1ULL && str->data == NULL case
  if (str->data != NULL) {
    iris_free(str->data);
  }
  string_move(str);
}

void string_move(IrisString* str) {
  str->data = NULL;
  str->len = 0ULL;
}

void string_print(const IrisString str, bool newline) {
  (void)fwrite((const void*)str.data, sizeof(char), str.len, stdout);
  if (newline) (void)fputc('\n', stdout);
  fflush(stdout);
}

void string_print_repr(const IrisString str, bool newline) {
  (void)fputc('"', stdout);
  (void)fwrite((const void*)str.data, sizeof(char), str.len, stdout);
  (void)fputc('"', stdout);
  if (newline) (void)fputc('\n', stdout);
  fflush(stdout);
}

void string_print_internal(const IrisString str, bool newline) {
  (void)fprintf(stdout, "<string | bytes: \"%.*s\" : len: %llu, hash: %llu)", (int)str.len, str.data, str.len, str.hash);
  if (newline) (void)fputc('\n', stdout);
  fflush(stdout);
}
