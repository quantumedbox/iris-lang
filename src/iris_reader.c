#include <types/iris_string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>

#include "iris_reader.h"
#include "types/iris_types.h"
#include "iris_utf8.h"
#include "iris_utils.h"

// todo: require spaces between in-list objects
// todo: there's no need to allocate 'quote' strings over and over again, for example, it's really wasteful
// todo: floats
// todo: catching errors from sub-parse functions

typedef enum {
  psAbort, // returned on failure without error
  psError, // returned on error, target has error object
  psResult // returned when target has valid object
} ParseStatus;

typedef ParseStatus (*ParseProc)(IrisObject* target, size_t* parsed, const char* slice, const char* limit);

static ParseStatus parse_quote(IrisObject*, size_t*, const char*, const char*);
static ParseStatus parse_int(IrisObject*, size_t*, const char*, const char*);
static ParseStatus parse_atomic_symbol(IrisObject*, size_t*, const char*, const char*);
static ParseStatus parse_marked_symbol(IrisObject*, size_t*, const char*, const char*);
static ParseStatus parse_list(IrisObject*, size_t*, const char*, const char*);

static const ParseProc parsing_procs[] = {
  parse_quote,
  parse_int,
  parse_atomic_symbol,
  parse_marked_symbol,
  parse_list
};

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
  @brief  Skip any whitespace characters, also skip comments
  @return How many characters are skipped
*/
static size_t eat_whitespace(const char* slice, const char* limit) {
  assert(slice <= limit);
  const char* ptr = slice;
  while ((ptr <= limit)) {
    if (is_whitespace(*ptr)) {
      ptr++;
    } else if (*ptr == ';') {
      do {
        ptr += utf8_codepoint_width(*ptr);
      } while (!ascii_cmp(*ptr, '\n'));
    } else {
      break;
    }
  }
  return (size_t)(((ptrdiff_t)ptr - (ptrdiff_t)slice) / sizeof(char));
}

static ParseStatus parse_int(IrisObject* target, size_t* parsed, const char* slice, const char* limit) {
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
    if (ptr > limit)
      return psAbort;
  }
  if ((utf8_codepoint_width(*ptr) == 1U) && (*ptr >= '1') && (*ptr <= '9')) {
    IrisObject result = { .kind = irisObjectKindInt, .int_variant = (*ptr - '0') };
    ptr++;
    while ((ptr <= limit) && (utf8_codepoint_width(*ptr) == 1U) && (*ptr >= '0') && (*ptr <= '9')) {
      intmax_t check = result.int_variant;
      result.int_variant = result.int_variant * 10 + (*ptr - '0');
      if (check >= result.int_variant) { // todo: lower bound truncates incorrectly
        *target = error_to_object(error_new(is_negative? irisErrorUnderflowError : irisErrorOverflowError));
        return psError;
      }
      ptr++;
    }
    if (is_negative) {
      result.int_variant = 0 - result.int_variant;
    }
    *target = result;
    *parsed = (size_t)(((ptrdiff_t)ptr - (ptrdiff_t)slice) / sizeof(char));
    return psResult;
  }
  return psAbort;
}

static ParseStatus parse_atomic_symbol(IrisObject* target, size_t* parsed, const char* slice, const char* limit) {
  assert(slice <= limit);
  assert(target != NULL);
  assert(parsed != NULL);
  const char* ptr = slice;
  while (ptr <= limit) {
    if (is_reserved_char(*ptr) || is_whitespace(*ptr)) {
      break;
    }
    ptr += utf8_codepoint_width(*ptr);
  }
  if (ptr == slice) 
    return psAbort;
  IrisObject result = string_to_object(string_from_view(slice, ptr));
  *target = result;
  *parsed = result.string_variant.len;
  return psResult;
}

static ParseStatus parse_marked_symbol(IrisObject* target, size_t* parsed, const char* slice, const char* limit) {
  assert(slice <= limit);
  assert(target != NULL);
  assert(parsed != NULL);
  const char* ptr = slice;
  if (ascii_cmp(*ptr, '\"')) {
    ptr++;
    while (ptr <= limit) {
      if (ascii_cmp(*ptr, '\"')) {
        IrisObject result = string_to_object(string_from_view(slice + 1U, ptr));
        *target = result;
        *parsed = result.string_variant.len + 2ULL;
        return psResult;
      }
      ptr += utf8_codepoint_width(*ptr);
    }
    *target = error_to_object(error_from_chars(irisErrorSyntaxError, "trailing unclosed string"));
    return psError;
  }
  return psAbort;
}

static ParseStatus parse_quote(IrisObject* target, size_t* parsed, const char* slice, const char* limit) {
  assert(slice <= limit);
  assert(target != NULL);
  assert(parsed != NULL);
  const char* ptr = slice;
  if (ascii_cmp(*ptr, '\'')) {
    ptr++; // skip past '\'
    if (ptr > limit) {
      *target = error_to_object(error_from_chars(irisErrorSyntaxError, "nothing to quote"));
      return psError;
    }
    ptr += eat_whitespace(ptr, limit);
    if (ptr > limit) {
      *target = error_to_object(error_from_chars(irisErrorSyntaxError, "nothing to quote"));
      return psError;
    }
    IrisList result = list_new();
    IrisString quote_sym = string_from_chars("quote!");
    list_push_string(&result, &quote_sym);
    IrisObject obj_parsed;
    size_t chars_parsed;
    if (parse_quote(&obj_parsed, &chars_parsed, ptr, limit) ||
        parse_int(&obj_parsed, &chars_parsed, ptr, limit) ||
        parse_atomic_symbol(&obj_parsed, &chars_parsed, ptr, limit) ||
        parse_marked_symbol(&obj_parsed, &chars_parsed, ptr, limit) ||
        parse_list(&obj_parsed, &chars_parsed, ptr, limit)) {
      list_push_object(&result, &obj_parsed);
      ptr += chars_parsed;
      *target = list_to_object(result);
      *parsed = (size_t)(((ptrdiff_t)ptr - (ptrdiff_t)slice) / sizeof(char));
      return psResult;
    } else {
      *target = error_to_object(error_from_chars(irisErrorSyntaxError, "unknown symbol sequence"));
      list_destroy(&result);
      return psError;
    }
  }
  return psAbort;
}

static ParseStatus parse_list(IrisObject* target, size_t* parsed, const char* slice, const char* limit) {
  assert(slice <= limit);
  assert(target != NULL);
  assert(parsed != NULL);
  const char* ptr = slice;
  if (ascii_cmp(*ptr, '(')) {
    ptr++; // skip past '('
    if (ptr > limit) {
      *target = error_to_object(error_from_chars(irisErrorSyntaxError, "trailing unclosed list"));
      return psError;
    }
    ptr += eat_whitespace(ptr, limit);
    if (ptr > limit) {
      *target = error_to_object(error_from_chars(irisErrorSyntaxError, "trailing unclosed list"));
      return psError;
    }
    IrisList result = list_new();
    IrisObject obj_parsed;
    size_t chars_parsed;
    while (ptr <= limit) {
      ptr += eat_whitespace(ptr, limit);
      if (ptr > limit) { break; }
      if (ascii_cmp(*ptr, ')')) {
        *target = list_to_object(result);
        *parsed = (size_t)(((ptrdiff_t)ptr - (ptrdiff_t)slice) / sizeof(char)) + 1ULL;
        return psResult;
      }
      ParseStatus status = psAbort;
      size_t i = 0ULL;
      while ((status == psAbort) && (i < (sizeof(parsing_procs) / sizeof(ParseProc)))) {
        status = parsing_procs[i](&obj_parsed, &chars_parsed, ptr, limit);
        if (status == psError) {
          *target = *target;
          list_destroy(&result);
          return psError;
        } else if (status == psResult) {
          list_push_object(&result, &obj_parsed);
          ptr += chars_parsed;
          break;
        }
        i++;
      }
    }
    list_destroy(&result);
    *target = error_to_object(error_from_chars(irisErrorSyntaxError, "trailing unclosed list"));
    return psError;
  }
  return psAbort;
}

IrisObject string_read(const IrisString source) {
  if (utf8_check_validity(source)) {
    IrisList result = list_new();
    if (source.len == 0ULL) {
      return list_to_object(result);
    }
    const char* limit = &source.data[source.len - 1ULL];
    const char* ptr = source.data;
    ptr += eat_whitespace(ptr, limit);
    while (ptr <= limit) {
      ptr += eat_whitespace(ptr, limit);
      if (ptr > limit) { break; }
      IrisObject obj_parsed;
      size_t chars_parsed;
      ParseStatus status = psAbort;
      size_t i = 0ULL;
      while ((status == psAbort) && (i < (sizeof(parsing_procs) / sizeof(ParseProc)))) {
        status = parsing_procs[i](&obj_parsed, &chars_parsed, ptr, limit);
        if (status == psError) {
          list_destroy(&result);
          return obj_parsed;
        } else if (status == psResult) {
          list_push_object(&result, &obj_parsed);
          ptr += chars_parsed;
          break;
        }
        i++;
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
        *target = func_call(resolved->func_variant, &list.items[1], list.len - 1ULL);
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
            object_destroy(&result);
            return resolved;
          }
          list_push_object(&result.list_variant, &resolved);
        }
        // todo: this applies to top-most module body too which isn't desirable
        //       could fix by implementing function exactly like this, but without checking, but it's not that good
        // if ((obj.list_variant.len > 0ULL) && (result.list_variant.items[0].kind != irisObjectKindFunc)) {
        //   object_destroy(&result);
        //   return error_to_object(error_from_chars(irisErrorNameError, "unknown function name"));
        // }
        return result;
      }
    }
    default: break;
  }
  return object_copy(obj);
}
