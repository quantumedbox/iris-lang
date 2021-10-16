// todo: define way of signaling errors in functions
//       we should not use global state errno-like methods, everything should be
//       encapsulated as much as possible
//       for that we could probably use some kind of error object that is catched by caller afterwards

// todo: (help)
// todo: (doc <symbol>)

#include "core/cimpl.c"

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
// #include <signal.h>
#include <stdlib.h>

#include "eval.h"
#include "types/types.h"
#include "reader.h"
#include "memory.h"
#include "utils.h"

#define IRIS_ARGUMENT_STACK_LIMIT 32

const char* eval_welcome_msg =
  "- Iris REPL -\n"
  "| version -- unspecified\n"
  "| enter (help) or (doc <name>) for getting info\n"
  "| ctrl+c or (quit) for exit\n";

// todo: problem with exiting the repl
static volatile bool repl_should_exit = false;
// static void user_interrupt_handler(int sig) {
//   (void)sig;
//   repl_should_exit = true;
// }

static IrisDict standard_scope;

// todo: standard shouldn't be in eval maybe?
void init_eval(void) {
  if (dict_is_valid(standard_scope)) {
    dict_destroy(&standard_scope);
  }
  standard_scope = scope_default();
}

const IrisDict* get_standard_scope_view(void) {
  assert(dict_is_valid(standard_scope));
  return &standard_scope;
}

void enter_repl(void) {
  // if (signal(SIGINT, user_interrupt_handler) == SIG_ERR) {
  //   iris_check_warn(true, "problem with setting up SIGINT handler for repl");
  // }
  IrisDict scope = scope_default();
  (void)fprintf(stdout, "%s", eval_welcome_msg);
  while (!repl_should_exit) {
    (void)fprintf(stdout, ">>> ");
    IrisString line = string_from_file_line(stdin);
    IrisList sprout = nurture(line);
    eval(&sprout, &scope, true);
    string_destroy(&line);
    list_destroy(&sprout);
  }
  // signal(SIGINT, SIG_DFL);
  dict_destroy(&scope);
}

// todo: should be initialized once and then shared by const pointer
IrisDict scope_default(void) {
  #define scope_default_push_func_to_scope(m_cfunc, m_symbol) {  \
    IrisFunc func = func_from_cfunc(m_cfunc);                    \
    IrisString symbol = string_from_chars(m_symbol);             \
    dict_push_func(&result, &symbol, &func);                     \
  }
  #define scope_default_push_macro_to_scope(m_cfunc, m_symbol) { \
    IrisFunc func = func_macro_from_cfunc(m_cfunc);              \
    IrisString symbol = string_from_chars(m_symbol);             \
    dict_push_func(&result, &symbol, &func);                     \
  }

  IrisDict result = dict_new();
  scope_default_push_func_to_scope(echo, "echo");
  scope_default_push_func_to_scope(quit, "quit");
  scope_default_push_func_to_scope(plus, "+");
  scope_default_push_macro_to_scope(quote, "quote");
  scope_default_push_macro_to_scope(repeat_eval, "repeat-eval");
  scope_default_push_macro_to_scope(timeit, "timeit");
  scope_default_push_macro_to_scope(reduce, "reduce");
  return result;
  #undef scope_default_push_func_to_scope
  #undef scope_default_push_macro_to_scope
}

// todo: define ways of scope modification
//       it could probably be done by special dicts that have back references to scope from which they inherit
// todo: it should be guaranteed that evaluation doesn't mutate anything in terms of passed scopes and objects
IrisObject eval_object(const IrisObject obj, const IrisDict* scope) {
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
            if (!supposedly_callable->func_variant.is_macro) {
              IrisObject argument_stack[IRIS_ARGUMENT_STACK_LIMIT];
              size_t argument_stack_len = 0ULL;
              // collect function parameters by evaluating everything else that is in list
              for (size_t i = 1ULL; i < obj.list_variant.len; i++) {
                assert(object_is_valid(obj.list_variant.items[i]));
                argument_stack[i - 1ULL] = eval_object(obj.list_variant.items[i], scope);
                argument_stack_len++;
              }
              // todo: check on errors from callee
              return func_call(supposedly_callable->func_variant, argument_stack, argument_stack_len);
            } else {
              // slice function parameters without evaluation
              return func_call(supposedly_callable->func_variant, &obj.list_variant.items[1], obj.list_variant.len - 1ULL);
            }
          } else {
            return error_to_object(error_from_chars(irisErrorTypeError, "can't call given object"));
          }
        } else {
          return error_to_object(error_from_chars(irisErrorNameError, "unknown symbol"));
        }
        break;
      }
      default: return error_to_object(error_from_chars(irisErrorTypeError, "leading item in callable list should be string"));
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
      object_print_repr(result, true);
    }
  }
}
