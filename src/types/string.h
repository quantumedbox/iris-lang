#ifndef IRIS_STRING_H
#define IRIS_STRING_H

#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

// todo: string builder for making strings from individual parts, could be particularly helpful for reporting

typedef struct _IrisString {
  // iris byte strings are immutable and not null terminated
  // they don't have eny encoding attached and are hashed on creation
  char*  data;
  size_t len;
  size_t hash;
} IrisString;

/*
  @brief  Create string object from null-terminated string
  @warn   Use only for const static strings in binary and don't mess with dynamic C strings
*/
IrisString string_from_chars(const char*);

/*
  @brief  Reads file buffer until '\n' character is encountered
*/
IrisString string_from_file_line(FILE*);

/*
  @brief  Exhausting reading of supplied file
*/
IrisString string_from_file(FILE*);

IrisString string_from_view(const char* low, const char* high);
bool string_is_valid(IrisString str);
bool string_is_empty(IrisString str);
void string_hash(IrisString* str);
bool string_compare(IrisString x, IrisString y);
char string_nth(IrisString, size_t idx);
void string_destroy(IrisString*);
void string_move(IrisString*);
void string_print_repr(IrisString, bool newline);
void string_print_internal(IrisString, bool newline);

#endif
