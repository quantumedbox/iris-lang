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

static IrisDict standard_scope;
static IrisList argv_list; // todo: should be placed in refcell and shared, otherwise it will be destroyed

// todo: problem with exiting the repl
static volatile bool repl_should_exit = false;
// static void user_interrupt_handler(int sig) {
//   (void)sig;
//   repl_should_exit = true;
// }

IrisDict scope_default(void) {
  IrisDict result = dict_new();

  #define push_to_scope(m_push_by, m_cfunc, m_symbol) {  \
    IrisFunc func = m_push_by(m_cfunc);                    \
    IrisString symbol = string_from_chars(m_symbol);             \
    dict_push_func(&result, &symbol, &func);                     \
  }
  // argv list
  IrisList argv_copy = list_copy(argv_list);
  IrisString argv_name = string_from_chars("argv"); // todo: kinda lame that we have to create strings for that, there should be way to hash chars 
  dict_push_list(&result, argv_name.hash, &argv_copy);
  string_destroy(&argv_name);

  // functions
  push_to_scope(func_macro_from_cfunc,  quote,        "quote");
  push_to_scope(func_from_cfunc,        echo,         "echo");
  push_to_scope(func_from_cfunc,        quit,         "quit");
  push_to_scope(func_from_cfunc,        add,          "+");
  push_to_scope(func_from_cfunc,        sub,          "-");
  push_to_scope(func_from_cfunc,        reduce,       "reduce");
  push_to_scope(func_macro_from_cfunc,  timeit,       "timeit");
  push_to_scope(func_macro_from_cfunc,  repeat_eval,  "repeat-eval");
  push_to_scope(func_from_cfunc,        metrics,      "metrics");

  return result;
  #undef push_to_scope
}

// todo: standard shouldn't be in eval maybe?
void eval_module_init(int argc, const char* argv[]) {
  if (dict_is_valid(standard_scope)) {
    dict_destroy(&standard_scope);
    warning("reconstruction of default scope dictionary");
  }
  if (list_is_valid(argv_list) && !list_is_empty(argv_list)) {
    list_destroy(&argv_list);
    warning("reconstruction of argv list");
  }
  argv_list = list_new();
  for (int i = 0; i < argc; i++) {
    IrisString str = string_from_chars(argv[i]);
    assert(string_is_valid(str));
    list_push_string(&argv_list, &str);
  }
  standard_scope = scope_default();
}

void eval_module_deinit(void) {
  if (dict_is_valid(standard_scope)) {
    dict_destroy(&standard_scope);
  } else {
    panic("default scope dictionary is ill-formed");
  }
  if (list_is_valid(argv_list)) {
    list_destroy(&argv_list);
  } else {
    panic("argv list is ill-formed");
  }
}

const IrisDict* get_standard_scope_view(void) {
  assert(dict_is_valid(standard_scope));
  return &standard_scope;
}

void enter_repl(void) {
  // if (signal(SIGINT, user_interrupt_handler) == SIG_ERR) {
  //   iris_check_warn(true, "problem with setting up SIGINT handler for repl");
  // }
  // IrisDict scope = scope_default();
  const IrisDict* scope = get_standard_scope_view();
  (void)fprintf(stdout, "%s", eval_welcome_msg);
  while (!repl_should_exit) {
    (void)fprintf(stdout, ">>> ");
    IrisString line = string_from_file_line(stdin);
    IrisList sprout = nurture(line);
    eval(&sprout, scope, true);
    string_destroy(&line);
    list_destroy(&sprout);
  }
  // signal(SIGINT, SIG_DFL);
}

// todo: define ways of scope modification
//       it could probably be done by special dicts that have back references to scope from which they inherit
// todo: it should be guaranteed that evaluation doesn't mutate anything in terms of passed scopes and objects
IrisObject eval_object(const IrisObject obj, const IrisDict* scope) {
  assert(obj.kind < N_OBJECT_KINDS && obj.kind >= 0);
  // non-empty lists are evaluated, you need to place them in quote list to get them as they are
  if ((obj.kind == irisObjectKindList) && (obj.list_variant.len > 0ULL)) {
    assert(object_is_valid(obj.list_variant.items[0]));
    IrisObject callable = eval_object(obj.list_variant.items[0], scope);
    assert(object_is_valid(callable));
    if (callable.kind == irisObjectKindError) {
      return callable;
    }
    if (callable.kind != irisObjectKindFunc) {
      return error_to_object(error_from_chars(irisErrorTypeError, "can't call non-function object"));
    }
    // if function is macro - don't evaluate its parameters and pass as is
    if (callable.func_variant.is_macro) {
      IrisObject result = func_call(callable.func_variant, &obj.list_variant.items[1], obj.list_variant.len - 1ULL);
      object_destroy(&callable);
      return result;
    // otherwise evaluate the parameters
    } else {
      IrisObject argument_stack[IRIS_ARGUMENT_STACK_LIMIT];
      for (size_t i = 1ULL; i < obj.list_variant.len; i++) {
        assert(object_is_valid(obj.list_variant.items[i]));
        argument_stack[i - 1ULL] = eval_object(obj.list_variant.items[i], scope);
        assert(object_is_valid(argument_stack[i - 1ULL]));
        if (argument_stack[i - 1ULL].kind == irisObjectKindError) {
          for (size_t free_i = 0ULL; free_i < i - 1ULL; free_i++) {
            object_destroy(&argument_stack[free_i]);
          }
          return argument_stack[i - 1ULL];
        }
      }
      IrisObject result = func_call(
        callable.func_variant,
        obj.list_variant.len > 1 ? &argument_stack[0] : NULL,
        obj.list_variant.len - 1ULL);
      object_destroy(&callable);
      return result;
    }
  } else if (obj.kind == irisObjectKindString) {
    assert(string_is_valid(obj.string_variant));
    if (dict_has(*scope, obj.string_variant.hash)) {
      const IrisObject* lookup = dict_get_view(scope, obj.string_variant.hash);
      assert(object_is_valid(*lookup));
      return *lookup;
    } else {
      return error_to_object(error_from_chars(irisErrorNameError, "unknown symbol"));
    }
  } else {
    return obj;
  }
}

void eval(const IrisList* list, const IrisDict* scope, bool in_repl) {
  assert(list_is_valid(*list) && dict_is_valid(*scope));
  for (size_t i = 0ULL; i < list->len; i++) {
    IrisObject result = eval_object(list->items[i], scope);
    if (in_repl == true) {
      object_print_repr(result, true);
      object_destroy(&result);
    }
  }
}
