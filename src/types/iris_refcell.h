#ifndef IRIS_REFCELL_H
#define IRIS_REFCELL_H

#include <stdbool.h>

// todo: store size of object to know the residence of counter in mem?
// todo: the fact that memory is untyped makes a lot of problem, maybe there's better way?

typedef struct _IrisRefCell {
  // Object wrapper that counts references and frees memory when counter reaches zero
  // Holded object should be totally immutable
  struct _IrisObject* ref;
  unsigned int* counter;
} IrisRefCell;

/*
  @brief  Create refcell from object
  @warn   Object should not be referenced by any refcell already!
*/
IrisRefCell refcell_from_object(struct _IrisObject*);

/*
  @brief  Get untyped view into memory, bytes is size of type that is stored
  @warn   Data under retrieved pointer should not be mutated!
*/
const struct _IrisObject* refcell_view(const IrisRefCell);

unsigned int refcell_refcount(const IrisRefCell);
IrisRefCell refcell_copy(const IrisRefCell);
void refcell_destroy(IrisRefCell*);
void refcell_move(IrisRefCell*);

/*
  @brief  Returns true if refcell points to valid memory
*/
bool refcell_is_valid(const IrisRefCell);

void refcell_print(const IrisRefCell, bool newline);
void refcell_print_repr(const IrisRefCell, bool newline);

#define refcell_to_object(ref) (IrisObject){ .kind = irisObjectKindRefCell, .refcell_variant = ref }

#endif
