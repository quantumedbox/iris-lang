#ifndef IRIS_UTF8_H
#define IRIS_UTF8_H

#include "iris_utils.h"

/*
  @brief  Check to what codepoint width given octet corresponds
  @warn   Should be valid starting UTF-8 octet
*/
__forceinline unsigned int utf8_codepoint_width(char ch) {
  if (!(ch & 0b10000000)) {
    return 1U;
  } else if (!(ch & 0b00100000) && ((ch & 0b11000000) == 0b11000000)) {
    return 2U;
  } else if (!(ch & 0b00010000) && ((ch & 0b11100000) == 0b11100000)) {
    return 3U;
  } else if (!(ch & 0b00001000) && ((ch & 0b11110000) == 0b11110000)) { // should we care about ISO/IEC 10646?
    return 4U;
  }
  panic("invalid utf8 codepoint");
}

// todo: skip BOM? windows is bitch with it
__forceinline bool utf8_check_validity(const IrisString str) {
  const char* ptr = str.data;
  while (ptr < &str.data[str.len]) {
    if ((*ptr & 0b10000000) & (!(*ptr & 0b01000000))) {
      // first bit cannot be 1 and then followed by 0
      return false;
    }
    unsigned int width = utf8_codepoint_width(*ptr);
    if ((ptr + width) > &str.data[str.len]) {
      // codepoint is outside of string bounds
      return false;
    }
    ptr++;
    for (unsigned int i = 0U; i < width - 1U; i++) {
      // each octet should start with 0b10xxxxxx
      if (!(*ptr & 0b10000000) || (*ptr & 0b01000000)) {
        return false;
      }
      ptr++;
    }
  }
  return true;
}

__forceinline size_t utf8_count_chars(const IrisString str) {
  size_t result = 0;
  const char* ptr = str.data;
  while (ptr < &str.data[str.len]) {
    ptr += utf8_codepoint_width(*ptr);
    result++;
  }
  return result;
}

#define ascii_cmp(ch, c_ch) (utf8_codepoint_width(ch) == 1U) && (ch == c_ch)

#endif
