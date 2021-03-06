#ifndef IRIS_LIST_H
#define IRIS_LIST_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct _IrisList {
  // iris lists are arrays which do act like typical lisp linked lists
  // by making elements stored in reversed order which allows O(1) insertions to 'head'
  // tho we don't really have to conform to this tradition even?
  // optionally we could just provide special function interface for interpreting lists as linked lists
  // but that's really not a good idea, but we will see
  struct _IrisObject* items;
  size_t len;
  size_t cap;
} IrisList;

IrisList list_new(void);
IrisList list_from_chars_array(int count, const char**);
IrisList list_copy(const IrisList);

/*
  @brief  Moves variant object to list
  @warn   Passed object should no longer be used!
*/
void list_push_object(IrisList*, struct _IrisObject*);

void list_push_int(IrisList*, intmax_t); // todo: should it be passed by ref to?
void list_push_string(IrisList*, struct _IrisString*);
void list_push_list(IrisList*, IrisList*);
bool list_is_valid(const IrisList);
bool list_is_empty(const IrisList);
size_t list_card(const IrisList);
bool list_is_valid(const IrisList);

/*
  @brief  Get pointer to internal IrisObject sequence
          It should be guaranteed that there's no way to mutate data by returned pointer
          And no way to use immutable IrisList for getting the slice
          Also caller supposed to get length of slice from function and not calculate it by themself
*/
// const struct _IrisObject* list_view_slice(const IrisList, size_t* resulting_slice_len, size_t l, size_t h);

/*
  @brief  Get list consisting of copy of consequent items in list
*/
struct _IrisList list_slice(const IrisList, size_t l, size_t h);

/*
  @brief  Push object to certain position replacing already existing one
*/
void list_nth_set(IrisList*, size_t idx, struct _IrisObject*);

size_t list_find(const IrisList, const struct _IrisObject);
bool list_has(const IrisList, const struct _IrisObject);

void list_destroy(IrisList*);
void list_move(IrisList*);
void list_print_repr(const IrisList, bool newline);
void list_print_internal(const IrisList, bool newline);

#define list_to_object(list) (struct _IrisObject){ .kind = irisObjectKindList, .list_variant = list }

#endif
