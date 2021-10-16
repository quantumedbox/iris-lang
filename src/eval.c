// todo: define way of signaling errors in functions
//       we should not use global state errno-like methods, everything should be
//       encapsulated as much as possible
//       for that we could probably use some kind of error object that is catched by caller afterwards

// todo: (help)
// todo: (doc <symbol>)

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <stdnoreturn.h>
#include <stdlib.h>

#include "eval.h"
#include "types/types.h"
#include "reader.h"
#include "utils.h"

#define IRIS_ARGUMENT_STACK_LIMIT 64

const char* eval_welcome_msg =
  "- Iris REPL -\n"
  "| version -- unspecified\n"
  "| enter (help) or (doc <name>) for getting info\n"
  "| ctrl+c or (quit) for exit\n";

// todo: problem with exiting the repl
static volatile bool repl_should_exit = false;
static void user_interrupt_handler(int sig) {
  (void)sig;
  repl_should_exit = true;
}

void enter_repl() {
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
  signal(SIGINT, SIG_DFL);
  dict_destroy(&scope);
}

/*
  @brief    Yields passed argument as it is, required for escaping evaluation
  @variants (1: any)
*/
static IrisObject quote(const IrisObject* args, size_t arg_count) {
  if (arg_count != 1ULL) {
    panic("invalid argument count for quote"); // should it not hard crush, but return error result?
  }
  return args[0]; // None
}

/*
  @brief    Exits the application
  @variants (0) (1: int)
*/
noreturn static IrisObject quit(const IrisObject* args, size_t arg_count) {
  if (arg_count > 1ULL) {
    panic("invalid argument count for exit"); // should it not hard crush, but return error result?
  } else if (arg_count == 0ULL) {
    exit(0);
  } else {
    iris_check(args[0].kind == irisObjectKindInt, "not int object passed to exit");
    exit(args[0].int_variant);
  }
}

/*
  @brief    Prints repr of objects into stdout
  @variants (n: any)
*/
static IrisObject echo(const IrisObject* args, size_t arg_count) {
  for (size_t i = 0ULL; i < arg_count; i++) {
    if (i != 0ULL) { (void)fputc(' ', stdout); }
    object_print_repr(args[i], false); // todo: object_repr?
  }
  (void)fputc('\n', stdout);
  return (IrisObject){0}; // None
}

// todo: should be initialized once and then shared by const pointer
IrisDict scope_default() {
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
  scope_default_push_macro_to_scope(quote, "quote");
  return result;
  #undef scope_default_push_func_to_scope
  #undef scope_default_push_macro_to_scope
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
            if (!supposedly_callable->func_variant.is_macro) {
              // collect function parameters by evaluating everything else that is in list
              for (size_t i = 1ULL; i < obj.list_variant.len; i++) {
                assert(object_is_valid(obj.list_variant.items[i]));
                argument_stack[i - 1ULL] = eval_object(obj.list_variant.items[i], scope);
                argument_stack_len++;
              }
            } else {
              // collect function parameters without evaluation
              for (size_t i = 1ULL; i < obj.list_variant.len; i++) {
                assert(object_is_valid(obj.list_variant.items[i]));
                argument_stack[i - 1ULL] = obj.list_variant.items[i];
                argument_stack_len++;
              }
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
      object_print_repr(result, true);
    }
  }
}
