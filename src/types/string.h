#ifndef IRIS_STRING_H
#define IRIS_STRING_H

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct _IrisString {
  // iris byte strings are immutable and not null terminated
  // they don't have eny encoding attached and are hashed on creation
  char*  data;
  size_t len;
  size_t hash;
} IrisString;

// #include "types/types.h"

IrisString string_from_chars(const char*);
IrisString string_from_file(FILE*);
IrisString string_from_view(const char* low, const char* high);
bool string_is_valid(IrisString str);
bool string_is_empty(IrisString str);
void string_hash(IrisString* str);
bool string_compare(IrisString x, IrisString y);
char string_nth(IrisString, size_t idx);
void string_destroy(IrisString*);
void string_move(IrisString*);
void string_print(IrisString, bool newline);
void string_print_debug(IrisString, bool newline);

#endif
