#include "eval.h"

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdnoreturn.h>
#include <stdlib.h>
#include <time.h>

#include "types/types.h"
#include "memory.h"
#include "utils.h"

// todo: they're bodged as hell, should standardize the way they're implemented
// todo: those things should be generated, not written manually. at least huge portion of it

/*
  @brief    Calls built-in memory metrics function
  @variants (0)
*/
static IrisObject metrics(const IrisObject* args, size_t arg_count) {
  iris_metrics_print_repr();
  return (IrisObject){0}; // None
}

/*
  @brief    Yields passed argument as it is, required for escaping evaluation
  @variants (1: any)
*/
static IrisObject quote(const IrisObject* args, size_t arg_count) {
  if (arg_count != 1ULL) {
    return error_to_object(error_from_chars(irisErrorContractViolation, "invalid argument count"));
  }
  return args[0];
}

// todo: should we not declare it as noreturn to have ability to return errors?
//       in my eyes quit should quit in any way
/*
  @brief    Exits the application
  @variants (0) (1: int)
*/
noreturn static IrisObject quit(const IrisObject* args, size_t arg_count) {
  iris_check(arg_count == 1ULL, "invalid argument count for exit"); // should it not hard crush, but return error result?
  if (arg_count == 0ULL) {
    exit(0);
  } else {
    iris_check(args[0].kind == irisObjectKindInt, "not int object passed to exit");
    exit(args[0].int_variant);
  }
}

// In-Iris alternative:
// >>> (defn echo [& args]
//        (apply print args))
/*
  @brief    Prints repr of objects into stdout
  @variants (n: any)
*/
static IrisObject echo(const IrisObject* args, size_t arg_count) {
  assert(((arg_count != 0ULL) && pointer_is_valid(args)) || (arg_count == 0));
  for (size_t i = 0ULL; i < arg_count; i++) {
    if (i != 0ULL) { (void)fputc(' ', stdout); }
    object_print_repr(args[i], false);
  }
  if (arg_count != 0ULL) (void)fputc('\n', stdout);
  return (IrisObject){0}; // None
}

// todo: it may leak memory if body tries to return allocated object
/*
  @brief    Macro for evaluating body n times, results of evaluations are dropped, returns None
  @variants (2: int body)
*/
static IrisObject repeat_eval(const IrisObject* args, size_t arg_count) {
  assert(pointer_is_valid(args));
  if (arg_count != 2ULL) {
    return error_to_object(error_from_chars(irisErrorContractViolation, "invalid argument count"));
  } else if (args[0].kind != irisObjectKindInt) {
    return error_to_object(error_from_chars(irisErrorTypeError, "first repeat argument should be int"));
  }
  const IrisDict* scope = get_standard_scope_view();
  for (int i = 0; i < args[0].int_variant; i++) {
    IrisObject something = eval_object(args[1], scope);
    catch_error(something);
    object_destroy(&something);
  }
  return (IrisObject){0}; // None
}

/*
  In-Iris alternative:
  >>> (defn-macro timeit [arg]
        (let [start (cputime)
              result (arg)]
          (echo "elapsed: " (- start (cputime)))
          result))
*/

// todo: should return result instead, time could be just printed
/*
  @brief    Macro for timing the execution of body
  @return   Float -- seconds of execution
  @variants (1: body)
*/
static IrisObject timeit(const IrisObject* args, size_t arg_count) {
  if (arg_count > 1ULL) {
    return error_to_object(error_from_chars(irisErrorContractViolation, "invalid argument count"));
  }
  assert(pointer_is_valid(args));
  assert(object_is_valid(args[0]));

  const IrisDict* scope = get_standard_scope_view();
  clock_t start_time = clock();
  IrisObject result = eval_object(args[0], scope);
  catch_error(result);
  clock_t end_time = clock();
  (void)fprintf(stdout, "time of execution: %f\n", (float)(end_time - start_time) / CLOCKS_PER_SEC);
  return result;
}

// In-Iris alternative:
// >>> (defn-macro reduce [fn ls]
//        (type-guard [fn func-t
//                     ls eval-t]
//          (let ))) ; todo
// todo: what a mess
/*
  @brief    Apply function to arguments with carry
  @return   Result of last function call
  @variants (2: func list)
*/
static IrisObject reduce(const IrisObject* args, size_t arg_count) {
  if (arg_count != 2ULL) {
    return error_to_object(error_from_chars(irisErrorContractViolation, "invalid argument count"));
  }
  assert(pointer_is_valid(args));
  assert(object_is_valid(args[0]));
  assert(object_is_valid(args[1]));
  if (args[0].kind != irisObjectKindFunc) {
    return error_to_object(error_from_chars(irisErrorTypeError, "first argument of reduce should be callable"));
  }
  const IrisDict* scope = get_standard_scope_view();
  IrisObject supposedly_list = eval_object(args[1], scope);
  catch_error(supposedly_list);
  if (supposedly_list.kind != irisObjectKindList) {
    return error_to_object(error_from_chars(irisErrorTypeError, "second argument of reduce should be list"));
  }
  if (supposedly_list.list_variant.len < 2ULL) {
    return error_to_object(error_from_chars(irisErrorTypeError, "there should be at least 2 arguments in list to be reduced"));
  }
  IrisObject cell[2] = {
    func_call(args[0].func_variant, &supposedly_list.list_variant.items[0], 2ULL),
    (IrisObject){0}
  };
  for (size_t i = 2ULL; i < supposedly_list.list_variant.len; i++) {
    cell[1] = supposedly_list.list_variant.items[i];
    cell[0] = func_call(args[0].func_variant, cell, 2ULL);
  }
  return cell[0];
}

/*
  @brief    Addition operation
            Promotes integers to floats if one of arg is float
  @return   Float | Int
  @variants (2: (float | int) (float | int))
*/
static IrisObject add(const IrisObject* args, size_t arg_count) {
  assert(pointer_is_valid(args));
  if (arg_count != 2ULL) {
    return error_to_object(error_from_chars(irisErrorContractViolation, "invalid argument count"));
  }
  if ((args[0].kind != irisObjectKindInt) && (args[0].kind != irisObjectKindFloat)) {
    return error_to_object(error_from_chars(irisErrorTypeError, "invalid argument"));
  }
  if ((args[1].kind != irisObjectKindInt) && (args[1].kind != irisObjectKindFloat)) {
    return error_to_object(error_from_chars(irisErrorTypeError, "invalid argument"));
  }
  if (args[0].kind != args[1].kind) {
    return (IrisObject){
      .kind = irisObjectKindFloat,
      .float_variant =
        ((args[0].kind == irisObjectKindInt) ? (float)args[0].int_variant : args[0].float_variant) +
        ((args[1].kind == irisObjectKindInt) ? (float)args[1].int_variant : args[1].float_variant)
    };
  } else {
    return (IrisObject){
      .kind = args[0].kind,
      .int_variant =
        ((args[0].kind == irisObjectKindInt) ? args[0].int_variant : args[0].float_variant) +
        ((args[1].kind == irisObjectKindInt) ? args[1].int_variant : args[1].float_variant)
    };
  }
}

/*
  @brief    Subtraction operation
            Promotes integers to floats if one of arg is float
  @return   Float | Int
  @variants (2: (float | int) (float | int))
*/
static IrisObject sub(const IrisObject* args, size_t arg_count) {
  assert(pointer_is_valid(args));
  if (arg_count != 2ULL) {
    return error_to_object(error_from_chars(irisErrorContractViolation, "invalid argument count"));
  }
  if ((args[0].kind != irisObjectKindInt) && (args[0].kind != irisObjectKindFloat)) {
    return error_to_object(error_from_chars(irisErrorTypeError, "invalid argument"));
  }
  if ((args[1].kind != irisObjectKindInt) && (args[1].kind != irisObjectKindFloat)) {
    return error_to_object(error_from_chars(irisErrorTypeError, "invalid argument"));
  }
  if (args[0].kind != args[1].kind) {
    return (IrisObject){
      .kind = irisObjectKindFloat,
      .float_variant =
        ((args[0].kind == irisObjectKindInt) ? (float)args[0].int_variant : args[0].float_variant) -
        ((args[1].kind == irisObjectKindInt) ? (float)args[1].int_variant : args[1].float_variant)
    };
  } else {
    return (IrisObject){
      .kind = args[0].kind,
      .int_variant =
        ((args[0].kind == irisObjectKindInt) ? args[0].int_variant : args[0].float_variant) -
        ((args[1].kind == irisObjectKindInt) ? args[1].int_variant : args[1].float_variant)
    };
  }
}
