#include <string.h>
#include <stdbool.h>
#include "iris.h"
#include "reader.h"

#define LIST_RECURSION_PARSE_LIMIT 1028 // for now it's more than enough

/*
  @brief  Skip any whitespace character
  @return How many characters were skipped
*/
size_t parse_whitespace(const char* slice) {
  const char* ptr = slice;
  while (true) {
    switch (*ptr++) {
      case 9: continue;
      case 10: continue;
      case 11: continue;
      case 12: continue;
      case 13: continue;
      case 32: continue;
      // case 160: continue;
      default: //
    }
    break;
  }
  return (size_t)(((ptrdiff_t)ptr - (ptrdiff_t)slice) / sizeof(char));
}

// enum ParseStateBit {
//   psSymbol  = 0x1,
//   psString  = 0x2,
//   psInt     = 0x4,
//   psQuote   = 0x8,
// };

IrisList read_string(IrisString str) {
  IrisList module = new_list(); // top-most lists are part of it
  IrisList* stack[LIST_RECURSION_PARSE_LIMIT];
  size_t stack_len = 0;
  size_t pos = 0;

}
