#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "reader.h"
#include "types/types.h"
#include "utils.h"

// todo: require spaces between in-list objects
// todo: quoted symbols
// todo: store symbols in global string pool
//       there's no need to allocate 'quote' strings over and over again, for example, it's really wasteful

#define LIST_RECURSION_PARSE_LIMIT 1028 // for now it's more than enough

__forceinline bool is_whitespace(char ch) {
  switch (ch) {
    case  9: return true;
    case 10: return true;
    case 11: return true;
    case 12: return true;
    case 13: return true;
    case 32: return true;
    default: return false;
  }
}

__forceinline bool is_reserved_char(char ch) {
  switch (ch) {
    case '\'': return true;
    case '\"': return true;
    case '(': return true;
    case ')': return true;
    default:   return false;
  }
}

/*
  @brief  Skip any whitespace character
  @return How many characters are skipped
*/
size_t eat_whitespace(const char* slice, const char* limit) {
  assert(slice <= limit);
  const char* ptr = slice;
  while (limit >= ptr) {
    if (is_whitespace(*ptr)) {
      ptr++;
    } else {
      break;
    }
  }
  return (size_t)(((ptrdiff_t)ptr - (ptrdiff_t)slice) / sizeof(char));
}

// todo: parse 0
// todo: parse negative
/*
  @brief  Try to parse iris integer from byte stream
          On success valid integer IrisObject variant will be written
          Also it's guaranteed that on failure pointed target will not change
  @return True on success, otherwise false
*/
bool parse_int(IrisObject* target, size_t* parsed, const char* slice, const char* limit) {
  assert(slice <= limit);
  assert(target != NULL);
  assert(parsed != NULL);
  const char* ptr = slice;
  if ((*ptr >= '1') && (*ptr <= '9')) {
    IrisObject result = { .kind = irisObjectKindInt, .int_variant = (*ptr - '0') };
    ptr++;
    while (limit >= ptr) {
      if ((*ptr >= '0') && (*ptr <= '9')) {
        int check = result.int_variant;
        result.int_variant = result.int_variant * 10 + (*ptr - '0');
        iris_check(check < result.int_variant, "can't contain given value in iris integer object (overflow detected)");
        ptr++;
      } else {
        break;
      }
    }
    *target = result;
    *parsed = (size_t)(((ptrdiff_t)ptr - (ptrdiff_t)slice) / sizeof(char));
    return true;
  }
  return false;
}

/*
  @brief  Try to parse iris symbol from byte stream
          Valid chars are anything but reserved symbols and whitespace
          On success valid symbol IrisObject variant will be written
          Also it's guaranteed that on failure pointed target will not change
  @return True on success, otherwise false
*/
bool parse_atomic_symbol(IrisObject* target, size_t* parsed, const char* slice, const char* limit) {
  assert(slice <= limit);
  assert(target != NULL);
  assert(parsed != NULL);
  const char* ptr = slice;
  IrisObject result = { .kind = irisObjectKindString };
  while (limit > ptr) {
    if (!is_reserved_char(*ptr) && !is_whitespace(*ptr)) {
      ptr++;
    } else {
      break;
    }
  }
  if (ptr == slice) {
    return false;
  }
  result.string_variant = string_from_view(slice, ptr);
  *target = result;
  *parsed = result.string_variant.len;
  return true;
}

/*
  @brief  Try to parse iris string from byte stream
          On success valid symbol IrisObject variant will be written
          Also it's guaranteed that on failure pointed target will not change
  @return True on success, otherwise false
*/
bool parse_marked_symbol(IrisObject* target, size_t* parsed, const char* slice, const char* limit) {
  assert(slice <= limit);
  assert(target != NULL);
  assert(parsed != NULL);
  const char* ptr = slice;
  if (*ptr == '\"') {
    IrisObject result = { .kind = irisObjectKindString };
    ptr++;
    while (limit > ptr) {
      if (*ptr != '\"') {
        ptr++;
      } else {
        result.string_variant = string_from_view(slice + 1, ptr);
        *target = result;
        *parsed = result.string_variant.len + 2LU;
        return true;
      }
    }
    panic("trailing unclosed string");
  }
  return false;
}

IrisList nurture(IrisString str) {
  IrisList result = list_new(); // top-most lists are part of it
  IrisList* stack[LIST_RECURSION_PARSE_LIMIT]; // points at lists that aren't finalized yet
  stack[0] = &result;
  size_t stack_pos = 0ULL;
  size_t str_pos = 0ULL;

  while (str_pos != str.len) {
    assert(str_pos < str.len);
    str_pos += eat_whitespace(&str.data[str_pos], &str.data[str.len]);
    if (str_pos == str.len) { break; }
    char cur = string_nth(str, str_pos);
    switch (cur) {
      case '(': {
        IrisList list = list_new();
        list_push_list(stack[stack_pos], &list);
        iris_check(stack_pos < LIST_RECURSION_PARSE_LIMIT, "scope stack overflow");
        stack[stack_pos + 1ULL] = &(stack[stack_pos]->items[stack[stack_pos]->len - 1].list_variant); // kinda fucked up
        stack_pos++;
        str_pos++;
        continue;
      }
      case ')': {
        iris_check(stack_pos > 0ULL, "attempt to close nonexistent list");
        stack_pos--;
        str_pos++;
        continue;
      }
      case ';': {
        do {
          str_pos++;
          if (string_nth(str, str_pos) == '\n') {
            str_pos++;
            break;
          }
        }
        while (str_pos < str.len);
        continue;
      }
      default: {
        IrisObject obj_parsed;
        size_t chars_parsed;
        if (parse_int(&obj_parsed, &chars_parsed, &str.data[str_pos], &str.data[str.len])) {
          list_push_int(stack[stack_pos], obj_parsed.int_variant);
          str_pos += chars_parsed;
        } else if (parse_marked_symbol(&obj_parsed, &chars_parsed, &str.data[str_pos], &str.data[str.len])) {
          list_push_string(stack[stack_pos], &obj_parsed.string_variant);
          str_pos += chars_parsed;
        } else if (parse_atomic_symbol(&obj_parsed, &chars_parsed, &str.data[str_pos], &str.data[str.len])) {
          list_push_string(stack[stack_pos], &obj_parsed.string_variant);
          str_pos += chars_parsed;
        } else {
          panic("unknown character");
        }
      }
    }
  }
  iris_check(stack_pos == 0ULL, "trailing unclosed list");
  return result;
}
