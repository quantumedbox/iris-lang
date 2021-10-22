#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>

#include "reader.h"
#include "types/types.h"
#include "utils.h"

// todo: maybe some compile-time options? such as ability to define custom symbols or turning off quoting and such
// todo: should operate within interpreter and thus have ability to return error objects
// todo: require spaces between in-list objects
//       there's no need to allocate 'quote' strings over and over again, for example, it's really wasteful
// todo: floats
//       for that there should be some utility for decoding utf-8, preferably lazily
// todo: problem with strings in quoted lists:
//       '(shit) -> ("shit") -- this works, but if you want to have string that contains spaces
//       '("shit shit") -> '(quote "shit shit") -- will produce quoted string instead
//       one possible solution is to require quoting by ' and "" strings will be just strings
//       ' "this" -- but it's not very ergonomic, especially when written as '"this"
// todo: function for checking if string consists of valid UTF-8 codepoints

// #define LIST_RECURSION_PARSE_LIMIT 1028 // for now it's more than enough, but might be problematic in the future

static bool parse_list(IrisObject*, size_t*, const char*, const char*);

/*
  @brief  Check to what codepoint width given octet corresponds
  @warn   Should be valid starting UTF-8 octet
*/
__forceinline unsigned int utf8_codepoint_width(char ch) {
  if (!(ch & 0b10000000)) {
    return 1U;
  } else if (!(ch & 0b00100000) && ((ch & 0b11000000) == 0b11000000)) {
    return 2U;
  } else if (!(ch & 0b0001000) && ((ch & 0b11100000) == 0b11100000)) {
    return 3U;
  } else if (!(ch & 0b0000100) && ((ch & 0b11110000) == 0b11110000)) { // should we care about ISO/IEC 10646?
    return 4U;
  }
  panic("invalid utf8 codepoint");
}

bool is_encoded_in_utf8(const IrisString str) {
  const char* ptr = str.data;
  while (ptr < &str.data[str.len]) {
    if ((*ptr & 0b1000000) & (!(*ptr & 0b01000000))) {
      // first bit cannot be 1 and then followed by 0
      return false;
    }
    unsigned int width = utf8_codepoint_width(*ptr) - 1U;
    ptr++;
    if ((ptr + width) > &str.data[str.len]) {
      // codepoint is outside of string bounds
      return false;
    }
    for (unsigned int i = 0U; i < width; i++) {
      // each octet should start with 0b10xxxxxx
      if (!(*(ptr++) & 0b1000000)) {
        return false;
      }
    }
  }
  return true;
}

#define ascii_cmp(ch, c_ch) (utf8_codepoint_width(ch) == 1U) && (ch == c_ch)

__forceinline bool is_whitespace(char ch) {
  if (utf8_codepoint_width(ch) != 1U) {
    return false;
  }

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
  if (utf8_codepoint_width(ch) != 1U) {
    return false;
  }
  switch (ch) {
    case '\'': return true;
    case '\"': return true;
    case '(': return true;
    case ')': return true;
    case ';': return true;
    default:   return false;
  }
}

// todo: should we care about Unicode whitespace characters?
/*
  @brief  Skip any whitespace character
  @return How many characters are skipped
*/
static size_t eat_whitespace(const char* slice, const char* limit) {
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

static bool parse_int(IrisObject* target, size_t* parsed, const char* slice, const char* limit) {
  // todo: this macro usage is kinda bad as it operates on variables from outside its definition
  assert(slice <= limit);
  assert(target != NULL);
  assert(parsed != NULL);
  const char* ptr = slice;
  if (ascii_cmp(*ptr, '0')) {
    *target = (IrisObject){ .kind = irisObjectKindInt, .int_variant = 0 };
    *parsed = 1ULL;
    return true;
  }
  bool is_negative = false;
  if (ascii_cmp(*ptr, '-')) {
    is_negative = true;
    ptr++;
  }
  if ((utf8_codepoint_width(*ptr) == 1U) && (*ptr >= '1') && (*ptr <= '9')) {
    IrisObject result = { .kind = irisObjectKindInt, .int_variant = (*ptr - '0') };
    ptr++;
    while ((ptr < limit) && (utf8_codepoint_width(*ptr) == 1U) && (*ptr >= '0') && (*ptr <= '9')) {
      intmax_t check = result.int_variant;
      result.int_variant = result.int_variant * 10 + (*ptr - '0');
      if (check >= result.int_variant) { // todo: lower bound truncates incorrectly
        *target = error_to_object(error_new(is_negative? irisErrorUnderflowError : irisErrorOverflowError));
        return false;
      }
      ptr++;
    }
    if (is_negative) {
      result.int_variant = 0 - result.int_variant;
    }
    *target = result;
    *parsed = (size_t)(((ptrdiff_t)ptr - (ptrdiff_t)slice) / sizeof(char));
    return true;
  }
  // *target = error_to_object(error_new(irisErrorSyntaxError));
  return false;
}

static bool parse_atomic_symbol(IrisObject* target, size_t* parsed, const char* slice, const char* limit) {
  assert(slice <= limit);
  assert(target != NULL);
  assert(parsed != NULL);
  const char* ptr = slice;
  while (limit > ptr) {
    if (is_reserved_char(*ptr) || is_whitespace(*ptr)) {
      break;
    }
    ptr += utf8_codepoint_width(*ptr);
  }
  if (ptr == slice) {
    return false;
  }
  IrisObject result = string_to_object(string_from_view(slice, ptr));
  *target = result;
  *parsed = result.string_variant.len;
  return true;
}

static bool parse_marked_symbol(IrisObject* target, size_t* parsed, const char* slice, const char* limit) {
  assert(slice <= limit);
  assert(target != NULL);
  assert(parsed != NULL);
  const char* ptr = slice;
  if (ascii_cmp(*ptr, '\"')) {
    ptr++;
    while (limit > ptr) {
      if (ascii_cmp(*ptr, '\"')) {
        IrisObject result = string_to_object(string_from_view(slice + 1U, ptr));
        *target = result;
        *parsed = result.string_variant.len + 2ULL;
        return true;
      }
      ptr += utf8_codepoint_width(*ptr);
    }
    *target = error_to_object(error_from_chars(irisErrorSyntaxError, "trailing unclosed string"));
    return false;
  }
  // *target = error_to_object(error_new(irisErrorSyntaxError))
  return false;
}

// todo: what about requiring only two quotes?
// static bool parse_raw_marker(const char* slice, const char* limit) {
//   if ((slice + 2) >= limit) {
//     return false;
//   } else if ((*slice == '\"') && (*(slice + 1) == '\"') && (*(slice + 2) == '\"')) {
//     return true;
//   } else {
//     return false;
//   }
// }

// static bool parse_marked_raw_symbol(IrisObject* target, size_t* parsed, const char* slice, const char* limit) {
//   assert(slice <= limit);
//   assert(target != NULL);
//   assert(parsed != NULL);
//   const char* ptr = slice;
//   if (parse_raw_marker(ptr, limit)) {
//     IrisObject result = { .kind = irisObjectKindString };
//     ptr++;
//     while (limit > ptr) {
//       if (!parse_raw_marker(ptr, limit)) {
//         ptr++;
//       } else {
//         result.string_variant = string_from_view(slice + 3, ptr);
//         *target = result;
//         *parsed = result.string_variant.len + 6LU;
//         return true;
//       }
//     }
//     panic("trailing unclosed string");
//   }
//   return false;
// }

static bool parse_quote(IrisObject* target, size_t* parsed, const char* slice, const char* limit) {
  assert(slice <= limit);
  assert(target != NULL);
  assert(parsed != NULL);
  const char* ptr = slice;
  if (ascii_cmp(*ptr, '\'')) {
    ptr++;
    IrisList result = list_new();
    IrisString quote_sym = string_from_chars("quote!");
    list_push_string(&result, &quote_sym);
    IrisObject obj_parsed;
    size_t chars_parsed;
    ptr += eat_whitespace(ptr, limit);
    if (parse_quote(&obj_parsed, &chars_parsed, ptr, limit) ||
        parse_int(&obj_parsed, &chars_parsed, ptr, limit) ||
        parse_atomic_symbol(&obj_parsed, &chars_parsed, ptr, limit) ||
        parse_marked_symbol(&obj_parsed, &chars_parsed, ptr, limit) ||
        parse_list(&obj_parsed, &chars_parsed, ptr, limit)) {
      list_push_object(&result, &obj_parsed);
      ptr += chars_parsed;
      *target = list_to_object(result);
      *parsed = (size_t)(((ptrdiff_t)ptr - (ptrdiff_t)slice) / sizeof(char));
      return true;
    } else {
      *target = error_to_object(error_new(irisErrorSyntaxError));
      list_destroy(&result);
      return false;
    }
  }
  // *target = error_to_object(error_new(irisErrorSyntaxError));
  return false;
}

static bool parse_list(IrisObject* target, size_t* parsed, const char* slice, const char* limit) {
  assert(slice <= limit);
  assert(target != NULL);
  assert(parsed != NULL);
  const char* ptr = slice;
  if (ascii_cmp(*ptr, '(')) {
    ptr++;
    IrisList result = list_new();
    while (ptr != limit) {
      assert(ptr < limit);
      ptr += eat_whitespace(ptr, limit);
      if (ascii_cmp(*ptr, ')')) {
        *target = list_to_object(result);
        *parsed = (size_t)(((ptrdiff_t)ptr - (ptrdiff_t)slice) / sizeof(char)) + 1ULL;
        return true;
      }
      IrisObject obj_parsed;
      size_t chars_parsed;
      if (parse_quote(&obj_parsed, &chars_parsed, ptr, limit) ||
          parse_int(&obj_parsed, &chars_parsed, ptr, limit) ||
          parse_atomic_symbol(&obj_parsed, &chars_parsed, ptr, limit) ||
          parse_marked_symbol(&obj_parsed, &chars_parsed, ptr, limit) ||
          parse_list(&obj_parsed, &chars_parsed, ptr, limit)) {
        list_push_object(&result, &obj_parsed);
        ptr += chars_parsed;
      } else {
        *target = error_to_object(error_new(irisErrorSyntaxError));
        list_destroy(&result);
        return false;
      }
    }
    list_destroy(&result);
    *target = error_to_object(error_new(irisErrorSyntaxError));
    return false;
  }
  // *target = error_to_object(error_new(irisErrorSyntaxError));
  return false;
}

IrisObject string_read(const IrisString source) {
  if (is_encoded_in_utf8(source)) {
    const char* limit = &source.data[source.len];
    const char* ptr = source.data;
    IrisList result = list_new();
    while (ptr != limit) {
      assert(ptr < limit);
      ptr += eat_whitespace(ptr, limit);
      IrisObject obj_parsed;
      size_t chars_parsed;
      if (parse_quote(&obj_parsed, &chars_parsed, ptr, limit) ||
          parse_int(&obj_parsed, &chars_parsed, ptr, limit) ||
          parse_atomic_symbol(&obj_parsed, &chars_parsed, ptr, limit) ||
          parse_marked_symbol(&obj_parsed, &chars_parsed, ptr, limit) ||
          parse_list(&obj_parsed, &chars_parsed, ptr, limit)) {
        list_push_object(&result, &obj_parsed);
        ptr += chars_parsed;
      } else {
        list_destroy(&result);
        return error_to_object(error_new(irisErrorSyntaxError)); // todo: how to return valuable errors from parse_* functions?
      }
    }
    return list_to_object(result);
  } else {
    return error_to_object(error_from_chars(irisErrorEncodingError, "source string isn't encoded in UTF-8"));
  }
}

static bool try_resolve_macro(IrisObject* target, const IrisList list, const IrisDict scope) {
  if (list.len != 0ULL) {
    IrisObject leading = list.items[0];
    if ((leading.kind == irisObjectKindString) && dict_has(scope, leading)) {
      const IrisObject* resolved = dict_get_view(scope, leading);
      if ((resolved->kind == irisObjectKindFunc) && (resolved->func_variant.is_macro == true)) {
        *target = func_call(resolved->func_variant, &list.items[1], list.len - 1ULL); // todo: what if this returns error?
        return true;
      }
    }
  }
  return false;
}

static bool try_resolve_name(IrisObject* target, const IrisString str, const IrisDict scope) {
  if (dict_has(scope, string_to_object(str)) == true) {
    *target = dict_get(scope, string_to_object(str)); 
    return true;
  }
  return false;
}

IrisObject codelist_resolve(const IrisObject obj, const IrisDict scope) {
  switch (obj.kind) {
    case irisObjectKindString: {
      IrisObject result;
      if (try_resolve_name(&result, obj.string_variant, scope) == true) {
        return result;
      }
      break;
    }
    case irisObjectKindList: {
      IrisObject result;
      if (try_resolve_macro(&result, obj.list_variant, scope) == true) {
        return result;
      } else {
        result = list_to_object(list_new());
        for (size_t i = 0ULL; i < obj.list_variant.len; i++) {
          IrisObject resolved = codelist_resolve(obj.list_variant.items[i], scope);
          if (resolved.kind == irisObjectKindError) {
            list_destroy(&result.list_variant);
            return resolved;
          }
          list_push_object(&result.list_variant, &resolved);
        }
        return result;
      }
      // break;
    }
    default: break;
  }
  return object_copy(obj);
}
