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

IrisString string_copy(const IrisString);

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
bool string_is_valid(const IrisString);
bool string_is_empty(const IrisString);
bool string_compare(const IrisString, const IrisString);
bool string_compare_chars(const IrisString, const char*);
size_t string_card(const IrisString);

/*
  @brief  Returns copy of char at given idx
  @warn   Make sure that idx is within bounds
*/
char string_nth(const IrisString, size_t idx);

bool string_equal(const IrisString, const IrisString);

void string_destroy(IrisString*);
void string_move(IrisString*);
void string_print(const IrisString, bool newline);
void string_print_repr(const IrisString, bool newline);
void string_print_internal(const IrisString, bool newline);

#define string_to_object(str) (struct _IrisObject){ .kind = irisObjectKindString, .string_variant = str }

#endif
