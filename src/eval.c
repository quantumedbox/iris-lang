// todo: define way of signaling errors in functions
//       we should not use global state errno-like methods, everything should be
//       encapsulated as much as possible
//       for that we could probably use some kind of error object that is catched by caller afterwards

#include <stdbool.h>
#include <assert.h>

#include "eval.h"
#include "types/types.h"
#include "utils.h"

#define IRIS_ARGUMENT_STACK_LIMIT 64

/*
  @brief      Yields passed argument as it is, required for escaping evaluation
  @variants   (1: any)
*/
IrisObject quote(const IrisObject* args, size_t arg_count) {
  if (arg_count != 0ULL) {
    panic("invalid argument count for quote"); // should it not hard crush, but return error result?
  }
  return (IrisObject){0}; // None
}

IrisObject echo(const IrisObject* args, size_t arg_count) {
  for (size_t i = 0ULL; i < arg_count; i++) {
    if (i != 0ULL) { (void)fputc(' ', stdout); }
    object_print(args[i], false); // todo: object_repr?
  }
  (void)fputc('\n', stdout);
  return (IrisObject){0}; // None
}

// todo: should be initialized once and then shared by const pointer
IrisDict scope_default() {
  #define macro_push_to_scope(m_cfunc, m_symbol) {  \
  IrisFunc func = func_from_cfunc(m_cfunc);         \
  IrisString symbol = string_from_chars(m_symbol);  \
  dict_push_func(&result, &symbol, &func);          \
  }

  IrisDict result = dict_new();
  macro_push_to_scope(echo, "echo");
  return result;
  #undef macro_push_to_scope
}

// todo: define ways of scope modification
//       it could probably be done by special dicts that have back references to scope from which they inherit
// todo: it should be guaranteed that evaluation doesn't mutate anything in terms of passed scopes and objects
IrisObject eval_object(IrisObject obj, const IrisDict* scope) {
  assert(obj.kind < N_OBJECT_KINDS && obj.kind >= 0);
  if ((obj.kind == irisObjectKindList) && (obj.list_variant.len > 0ULL)) {
    // lists are evaluated as calls unless quoted
    IrisObject leading = obj.list_variant.items[0];
    assert(object_is_valid(leading));
    switch (leading.kind) {
      case irisObjectKindString: {
        // if first item in list is string then it's used for lookup of callable object
        if (dict_has(*scope, leading.string_variant.hash)) {
          const IrisObject* supposedly_callable = dict_get_view(scope, leading.string_variant.hash);
          // todo: object_is_callable? tho we probably should use func type for everything
          if (supposedly_callable->kind == irisObjectKindFunc) {
            // todo: that's quite a lot of stack consumption each call, we should find another way
            //       we could create our own stack in heap specifically for building calls
            IrisObject argument_stack[IRIS_ARGUMENT_STACK_LIMIT];
            size_t argument_stack_len = 0ULL;
            for (size_t i = 1ULL; i < obj.list_variant.len; i++) {
              // collect function parameters by evaluating everything else that is in list
              assert(object_is_valid(obj.list_variant.items[i]));
              argument_stack[i - 1ULL] = eval_object(obj.list_variant.items[i], scope);
              argument_stack_len++;
            }
            // todo: check on errors from callee
            return func_call(supposedly_callable->func_variant, argument_stack, argument_stack_len);
          } else {
            panic("looked up object isn't callable");
          }
        } else {
          panic("unknown symbol for list call");
        }
        break;
      }
      default:
        panic("invalid first parameter of callable list, should be symbol or func"); // todo: funcs aren't supported yet
    }
  } else {
    // everything else is equal to itself
    return obj;
  }
}

void eval(const IrisList* list, const IrisDict* scope, bool in_repl) {
  assert(list_is_valid(*list) && dict_is_valid(*scope));
  for (size_t i = 0ULL; i < list->len; i++) {
    IrisObject result = eval_object(list->items[i], scope);
    if (in_repl == true) {
      object_print(result, true);
    }
  }
}
