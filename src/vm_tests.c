#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "lexer.h"
#include "token.h"
#include "object_test_utils.h"
#include "parser.h"
#include "test_utils.h"
#include "vm.h"

typedef struct vm_testcase {
    const char *input;
    monkey_object_t *expected;
} vm_testcase;


static void
dump_bytecode(bytecode_t *bytecode)
{
    monkey_compiled_fn_t *fn;
    monkey_int_t *int_obj;
    if (bytecode->constants_pool == NULL)
        return;
    for (size_t i = 0; i < bytecode->constants_pool->length; i++) {
        monkey_object_t *constant = (monkey_object_t *) cm_array_list_get(bytecode->constants_pool, i);
        printf("CONSTANT %zu %p %s:\n", i, constant, get_type_name(constant->type));
        switch (constant->type) {
        case MONKEY_COMPILED_FUNCTION:
            fn = (monkey_compiled_fn_t *) constant;
            printf(" Instructions:\n%s", instructions_to_string(fn->instructions));
            break;
        case MONKEY_INT:
            int_obj = (monkey_int_t *) constant;
            printf(" Value: %ld\n", int_obj->value);
            break;
        default:
            break;
        }
        printf("\n");
    }
}

static void
run_vm_tests(size_t test_count, vm_testcase test_cases[test_count])
{
    for (size_t i = 0; i < test_count; i++) {
        vm_testcase t = test_cases[i];
        printf("Testing vm test for input %s\n", t.input);
        lexer_t *lexer = lexer_init(t.input);
        parser_t *parser = parser_init(lexer);
        program_t *program = parse_program(parser);
        compiler_t *compiler = compiler_init();
        compiler_error_t error = compile(compiler, (node_t *) program);
        if (error.code != COMPILER_ERROR_NONE)
            errx(EXIT_FAILURE, "compilation failed for input %s with error %s\n",
                t.input, error.msg);
        bytecode_t *bytecode = get_bytecode(compiler);
        vm_t *vm = vm_init(bytecode);
        vm_error_t vm_error = vm_run(vm);
        if (vm_error.code != VM_ERROR_NONE)
            errx(EXIT_FAILURE, "vm error: %s\n", vm_error.msg);
        monkey_object_t *top = vm_last_popped_stack_elem(vm);
        test_monkey_object(top, t.expected);
        free_monkey_object(top);
        parser_free(parser);
        program_free(program);
        compiler_free(compiler);
        bytecode_free(bytecode);
        vm_free(vm);
    }
}

static void
test_recursive_closures(void)
{
    vm_testcase tests[] = {
        {
            "let countDown = fn(x) {\n"
            "   if (x == 0) {\n"
            "       return 0\n"
            "   } else {\n"
            "       return countDown(x - 1);\n"
            "   }\n"
            "}\n"
            "countDown(1);\n",
            (monkey_object_t *) create_monkey_int(0)
        },
        {
            "let countDown = fn(x) {\n"
            "   if (x == 0) {\n"
            "       return 0\n"
            "   } else {\n"
            "       return countDown(x - 1);\n"
            "   }\n"
            "};\n"
            "let wrapper = fn() {\n"
            "   countDown(1);\n"
            "};\n"
            "wrapper();",
            (monkey_object_t *) create_monkey_int(0)
        },
        {
            "let wrapper = fn() {\n"
            "   let countDown = fn(x) {\n"
            "       if (x == 0) {\n"
            "           return 0;\n"
            "       } else {\n"
            "           return countDown(x - 1);\n"
            "       }\n"
            "   };\n"
            "   countDown(1);\n"
            "};\n"
            "wrapper();",
            (monkey_object_t *) create_monkey_int(0)
        }
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);

}

static void
test_recursive_fibonacci(void)
{
    vm_testcase tests[] = {
        {
            "let fibonacci = fn(x) {\n"
            "   if (x == 0) {\n"
            "       return 0;\n"
            "   } else {\n"
            "       if (x == 1) {\n"
            "           return 1;\n"
            "       } else {\n"
            "           fibonacci(x - 1) + fibonacci(x - 2);\n"
            "       }\n"
            "   }\n"
            "};\n"
            "fibonacci(15);",
            (monkey_object_t *) create_monkey_int(610)
        }
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);

}

static void
test_closures(void)
{
    vm_testcase tests[] = {
        {
            "let newClosure = fn(a) {\n"
            "   fn() {a;}\n"
            "};\n"
            "let closure = newClosure(99);\n"
            "closure();",
            (monkey_object_t *) create_monkey_int(99)
        },
        {
            "let newAdder = fn(a, b) {\n"
            "   fn(c) {a + b + c;};\n"
            "}\n"
            "let adder = newAdder(1, 2);\n"
            "adder(8);",
            (monkey_object_t *) create_monkey_int(11)
        },
        {
            "let newAdder = fn(a, b) {\n"
            "   let c = a + b;\n"
            "   fn(d) { c + d };\n"
            "}\n"
            "let adder = newAdder(1, 2);\n"
            "adder(8);",
            (monkey_object_t *) create_monkey_int(11)
        },
        {
            "let newAdderOuter = fn(a, b) {\n"
            "   let c = a + b;\n"
            "   fn(d) {\n"
            "       let e = d + c;\n"
            "       fn(f) {\n"
            "           e + f;\n"
            "       }\n"
            "   }\n"
            "}\n"
            "let newAdderInner = newAdderOuter(1, 2);\n"
            "let adder = newAdderInner(3);\n"
            "adder(8);\n",
            (monkey_object_t *) create_monkey_int(14)
        },
        {
            "let a = 1;\n"
            "let newAdderOuter = fn(b) {\n"
            "   fn(c) {\n"
            "       fn(d) { a + b + c + d;}\n"
            "   }\n"
            "};\n"
            "let newAdderInner = newAdderOuter(2);\n"
            "let adder = newAdderInner(3);\n"
            "adder(8);",
            (monkey_object_t *) create_monkey_int(14)
        },
        {
            "let newClosure = fn(a, b) {\n"
            "   let one = fn() {a;}\n"
            "   let two = fn() {b;}\n"
            "   fn() {one() + two();};\n"
            "};\n"
            "let closure = newClosure(9, 90);\n"
            "closure();",
            (monkey_object_t *) create_monkey_int(99)
        }
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static void
test_boolean_expressions(void)
{
    vm_testcase tests[] = {
        {"true", (monkey_object_t *) create_monkey_bool(true)},
        {"false", (monkey_object_t *) create_monkey_bool(false)},
        {"1 < 2", (monkey_object_t *) create_monkey_bool(true)},
        {"1 > 2", (monkey_object_t *) create_monkey_bool(false)},
        {"1 < 1", (monkey_object_t *) create_monkey_bool(false)},
        {"1 > 1", (monkey_object_t *) create_monkey_bool(false)},
        {"1 == 1", (monkey_object_t *) create_monkey_bool(true)},
        {"1 != 1", (monkey_object_t *) create_monkey_bool(false)},
        {"1 == 2", (monkey_object_t *) create_monkey_bool(false)},
        {"1 != 2", (monkey_object_t *) create_monkey_bool(true)},
        {"true == true", (monkey_object_t *) create_monkey_bool(true)},
        {"false == false", (monkey_object_t *) create_monkey_bool(true)},
        {"true == false", (monkey_object_t *) create_monkey_bool(false)},
        {"false != true", (monkey_object_t *) create_monkey_bool(true)},
        {"(1 < 2) == true", (monkey_object_t *) create_monkey_bool(true)},
        {"(1 < 2) == false", (monkey_object_t *) create_monkey_bool(false)},
        {"(1 > 2) == false", (monkey_object_t *) create_monkey_bool(true)},
        {"(1 > 2) == true", (monkey_object_t *) create_monkey_bool(false)},
        {"!(if (false) {5;})", (monkey_object_t *) create_monkey_bool(true)},
        {"if (if (false) {10}) {10} else {20}", (monkey_object_t *) create_monkey_int(20)}
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static void
test_integer_aritmetic(void)
{
    vm_testcase tests[] = {
        {"1", (monkey_object_t *) create_monkey_int(1)},
        {"2", (monkey_object_t *) create_monkey_int(2)},
        {"1 + 2", (monkey_object_t *) create_monkey_int(3)},
        {"1 - 2", (monkey_object_t *) create_monkey_int(-1)},
        {"1 * 2", (monkey_object_t *) create_monkey_int(2)},
        {"4 / 2", (monkey_object_t *) create_monkey_int(2)},
        {"50 / 2 * 2 + 10 - 5", (monkey_object_t *) create_monkey_int(55)},
        {"5 + 5 + 5 + 5 - 10", (monkey_object_t *) create_monkey_int(10)},
        {"2 * 2 * 2 * 2 * 2", (monkey_object_t *) create_monkey_int(32)},
        {"5 * 2 + 10", (monkey_object_t *) create_monkey_int(20)},
        {"5 + 2 * 10", (monkey_object_t *) create_monkey_int(25)},
        {"5 * (2 + 10)", (monkey_object_t *) create_monkey_int(60)},
        {"-5", (monkey_object_t *) create_monkey_int(-5)},
        {"-10", (monkey_object_t *) create_monkey_int(-10)},
        {"-50 + 100 + -50", (monkey_object_t *) create_monkey_int(0)},
        {"(5 + 10 * 2 + 15 / 3) * 2 + -10", (monkey_object_t *) create_monkey_int(50)}
    };

    print_test_separator_line();
    printf("Testing vm for integer arithmetic\n");
    size_t ntests = sizeof(tests)/ sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static void
test_conditionals(void)
{
    vm_testcase tests[] = {
        {"if (true) {10}", (monkey_object_t *) create_monkey_int(10)},
        {"if (true) {10} else {20}", (monkey_object_t *) create_monkey_int(10)},
        {"if (false) {10} else {20}", (monkey_object_t *) create_monkey_int(20)},
        {"if (1) {10}", (monkey_object_t *) create_monkey_int(10)},
        {"if (1 < 2) {10}", (monkey_object_t *) create_monkey_int(10)},
        {"if (1 < 2) {10} else {20}", (monkey_object_t *) create_monkey_int(10)},
        {"if (1 > 2) {10} else {20}", (monkey_object_t *) create_monkey_int(20)},
        {"if (false) {10}", (monkey_object_t *) create_monkey_null()},
        {"if (1 > 2) {10}", (monkey_object_t *) create_monkey_null()}
    };
    print_test_separator_line();
    printf("Testing conditionals\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static void
test_global_let_stmts(void)
{
    vm_testcase tests[] = {
        {"let one = 1; one", (monkey_object_t *) create_monkey_int(1)},
        {"let one = 1; let two = 2; one + two", (monkey_object_t *) create_monkey_int(3)},
        {"let one = 1; let two = one + one; one + two", (monkey_object_t *) create_monkey_int(3)}
    };
    print_test_separator_line();
    printf("Testing global let statements\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static void
test_string_expressions(void)
{
    vm_testcase tests[] = {
        {"\"monkey\"", (monkey_object_t *) create_monkey_string("monkey", 6)},
        {"\"mon\" + \"key\"", (monkey_object_t *) create_monkey_string("monkey", 6)},
        {"\"mon\" + \"key\" + \"banana\"", (monkey_object_t *) create_monkey_string("monkeybanana", 12)}
    };
    print_test_separator_line();
    printf("Testing string expressions\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static monkey_array_t *
create_monkey_int_array(size_t count, ...)
{
    va_list ap;
    va_start(ap, count);
    cm_array_list *list = cm_array_list_init(count, free_monkey_object);
    for (size_t i = 0; i < count; i++) {
        int val = va_arg(ap, int);
        cm_array_list_add(list, create_monkey_int(val));
    };
    va_end(ap);
    return create_monkey_array(list);
}

static void
test_array_literals(void)
{
    vm_testcase tests[] = {
        {"[]", (monkey_object_t *) create_monkey_int_array(0)},
        {"[1, 2, 3]", (monkey_object_t *) create_monkey_int_array(3, 1, 2, 3)},
        {"[1 + 2, 3  * 4, 5 + 6]", (monkey_object_t *) create_monkey_int_array(3, 3, 12, 11)}
    };
    print_test_separator_line();
    printf("Testing array literals\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static monkey_hash_t *
create_hash_table(size_t n, monkey_object_t *objects[n])
{
    cm_hash_table *table = cm_hash_table_init(monkey_object_hash, monkey_object_equals, free_monkey_object, free_monkey_object);
    for (size_t i = 0; i < n; i += 2) {
        monkey_object_t *key = objects[i];
        monkey_object_t *value = objects[i + 1];
        cm_hash_table_put(table, key, value);
    }
    return create_monkey_hash(table);
}

static void
test_hash_literals(void)
{
    vm_testcase tests[] = {
        {"{}", (monkey_object_t *) create_hash_table(0, NULL)},
        {"{1: 2, 3: 4}", (monkey_object_t *) create_hash_table((size_t) 4, (monkey_object_t *[4])
            {
                (monkey_object_t *) create_monkey_int(1),
                (monkey_object_t *) create_monkey_int(2),
                (monkey_object_t *) create_monkey_int(3),
                (monkey_object_t *) create_monkey_int(4)
            })
        },
        {"{1 + 1: 2 * 2, 3 + 3: 4 * 4}", (monkey_object_t *) create_hash_table((size_t) 4, (monkey_object_t *[4])
            {
                (monkey_object_t *) create_monkey_int(2),
                (monkey_object_t *) create_monkey_int(4),
                (monkey_object_t *) create_monkey_int(6),
                (monkey_object_t *) create_monkey_int(16)
            })
        }
    };
    print_test_separator_line();
    printf("Testing hash literals\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static void
test_index_expresions(void)
{
    vm_testcase tests[] = {
        {"[1, 2, 3][1]", (monkey_object_t *) create_monkey_int(2)},
        {"[1, 2, 3][0 + 2]", (monkey_object_t *) create_monkey_int(3)},
        {"[[1, 1, 1]][0][0]", (monkey_object_t *) create_monkey_int(1)},
        {"[][0]", (monkey_object_t *) create_monkey_null()},
        {"[1, 2, 3][99]", (monkey_object_t *) create_monkey_null()},
        {"[1][-1]", (monkey_object_t *) create_monkey_null()},
        {"{1: 1, 2: 2}[1]", (monkey_object_t *) create_monkey_int(1)},
        {"{1: 1, 2: 2}[2]", (monkey_object_t *) create_monkey_int(2)},
        {"{1: 1}[0]", (monkey_object_t *) create_monkey_null()},
        {"{}[0]", (monkey_object_t *) create_monkey_null()}
    };
    print_test_separator_line();
    printf("Testing index expressions\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static void
test_functions_without_arguments(void)
{
    vm_testcase tests[] = {
        {"let fivePlusTen = fn() {5 + 10;}; fivePlusTen();", (monkey_object_t *) create_monkey_int(15)},
        {"let one = fn() {1;}\n let two = fn() {2;}\n one() + two();", (monkey_object_t *) create_monkey_int(3)},
        {"let a = fn() {1};\n let b = fn() {a() + 1};\n let c = fn() {b() + 1;};\n c();", (monkey_object_t *) create_monkey_int(3)}
    };
    print_test_separator_line();
    printf("Testing functions without arguments\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static void
test_function_with_return_statement(void)
{
    vm_testcase tests[] = {
        {"let earlyExit = fn() {return 99; 100;};\n earlyExit();", (monkey_object_t *) create_monkey_int(99)},
        {"let earlyExit = fn() {return 99; return 100;};\n earlyExit();", (monkey_object_t *) create_monkey_int(99)}
    };
    print_test_separator_line();
    printf("Testing functions with return statement\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static void
test_functions_without_return_value(void)
{
    vm_testcase tests[] = {
        {"let noReturn = fn() {};\n noReturn();", (monkey_object_t *) create_monkey_null()},
        {"let noReturn = fn() {};\n let noReturnTwo = fn() {noReturn();}\n noReturn();\n noReturnTwo();", (monkey_object_t *) create_monkey_null()}
    };
    print_test_separator_line();
    printf("Testing functions without return value\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static void
test_first_class_functions(void)
{
    vm_testcase tests[] = {
        {"let returnOne = fn() {1;};\n let returnOneReturner = fn() {returnOne;};\n returnOneReturner()();",
            (monkey_object_t *) create_monkey_int(1)}
    };
    print_test_separator_line();
    printf("Testing first class functions\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static void
test_calling_functions_with_wrong_arguments(void)
{
    typedef struct testcase {
        const char *input;
        const char *expected_errmsg;
    } testcase;
    print_test_separator_line();
    printf("Testing functions with wrong arguments\n");

    testcase tests[] = {
        {
            "fn() {1;}(1);",
            "wrong number of arguments: want=0, got=1"
        },
        {
            "fn(a) {a;}();",
            "wrong number of arguments: want=1, got=0"
        },
        {
            "fn(a, b) {a + b;}(1);",
            "wrong number of arguments: want=2, got=1"
        }
    };

    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        testcase t = tests[i];
        printf("Testing %s\n", t.input);
        lexer_t *lexer = lexer_init(t.input);
        parser_t *parser = parser_init(lexer);
        program_t *program = parse_program(parser);
        compiler_t *compiler = compiler_init();
        compiler_error_t error = compile(compiler, (node_t *) program);
        if (error.code != COMPILER_ERROR_NONE)
            errx(EXIT_FAILURE, "compilation failed for input %s with error %s\n",
                t.input, error.msg);
        bytecode_t *bytecode = get_bytecode(compiler);
        vm_t *vm = vm_init(bytecode);
        vm_error_t vm_error = vm_run(vm);
        test(vm_error.code != VM_ERROR_NONE, "expected VM error but got no error\n");
        test(strcmp(vm_error.msg, t.expected_errmsg) == 0, "Expected error: %s, got %s\n", t.expected_errmsg, vm_error.msg);
        free(vm_error.msg);
        parser_free(parser);
        program_free(program);
        compiler_free(compiler);
        bytecode_free(bytecode);
        vm_free(vm);
    }
}

static void
test_calling_functions_with_bindings_and_arguments(void)
{
    vm_testcase tests[] = {
        {
            "let identity = fn(a) {a};\n"
            "identity(4);",
            (monkey_object_t *) create_monkey_int(4)
        },
        {
            "let sum = fn(a, b) { a + b;};\n"
            "sum(1, 2);",
            (monkey_object_t *) create_monkey_int(3)
        },
        {
            "let sum = fn(a, b) {\n"
            "  let c = a + b;\n"
            "  c;\n"
            "};\n"
            "sum(1, 2);",
            (monkey_object_t *) create_monkey_int(3)
        },
        {
            "let sum = fn(a, b) {\n"
            "  let c = a + b;\n"
            "  c;\n"
            "};\n"
            "sum(1, 2) + sum(3, 4);",
            (monkey_object_t *) create_monkey_int(10)
        },
        {
            "let sum = fn(a, b) {\n"
            "  let c = a + b;\n"
            "  c;\n"
            "};\n"
            "let outer = fn() {\n"
            "  sum(1, 2) + sum(3, 4);\n"
            "};\n"
            "outer();",
            (monkey_object_t *) create_monkey_int(10)
        },
        {
            "let globalNum = 10;\n"
            "let sum = fn(a, b) {\n"
            "  let c = a + b;\n"
            "  c + globalNum;\n"
            "};\n"
            "let outer = fn() {\n"
            "  sum(1, 2) + sum(3, 4) + globalNum;\n"
            "};\n"
            "outer() + globalNum;",
            (monkey_object_t *) create_monkey_int(50)
        }
    };
    print_test_separator_line();
    printf("Testing functions with bindings and arguments\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static monkey_array_t *
create_int_array(int *int_arr, size_t length)
{
    cm_array_list *array_list = cm_array_list_init(length, free_monkey_object);
    for (size_t i = 0; i < length; i++) {
        cm_array_list_add(array_list, (void *) create_monkey_int(int_arr[i]));
    }
    return create_monkey_array(array_list);
}

static void
test_builtin_functions(void)
{
    vm_testcase tests[] = {
        {
            "len(\"\")",
            (monkey_object_t *) create_monkey_int(0)
        },
        {
            "len(\"four\")",
            (monkey_object_t *) create_monkey_int(4)
        },
        {
            "len(\"hello world\")",
            (monkey_object_t *) create_monkey_int(11)
        },
        {
            "len(1)",
            (monkey_object_t *) create_monkey_error("argument to `len` not supported, got INTEGER")
        },
        {
            "len(\"one\", \"two\")",
            (monkey_object_t *) create_monkey_error("wrong number of arguments. got=2, want=1")
        },
        {
            "len([1, 2, 3])",
            (monkey_object_t *) create_monkey_int(3)
        },
        {
            "len([])",
            (monkey_object_t *) create_monkey_int(0)
        },
        {
            "puts(\"hello\", \"world!\")",
            (monkey_object_t *) create_monkey_null()
        },
        {
            "first([1, 2, 3])",
            (monkey_object_t *) create_monkey_int(1)
        },
        {
            "first([])",
            (monkey_object_t *) create_monkey_null()
        },
        {
            "first(1)",
            (monkey_object_t *) create_monkey_error("argument to `first` must be ARRAY, got INTEGER")
        },
        {
            "last([1, 2, 3])",
            (monkey_object_t *) create_monkey_int(3)
        },
        {
            "last([])",
            (monkey_object_t *) create_monkey_null()
        },
        {
            "last(1)",
            (monkey_object_t *) create_monkey_error("argument to `last` must be ARRAY, got INTEGER")
        },
        {
            "rest([1, 2, 3])",
            (monkey_object_t *) create_int_array((int[]) {2, 3}, 2)
        },
        {
            "rest([])",
            (monkey_object_t *) create_monkey_null()
        },
        {
            "push([], 1)",
            (monkey_object_t *) create_int_array((int[]) {1}, 1)
        },
        {
            "push(1, 1)",
            (monkey_object_t *) create_monkey_error("argument to `push` must be ARRAY, got INTEGER")
        }
    };
    print_test_separator_line();
    printf("Testing builtin functions\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

static void
test_calling_functions_with_bindings(void)
{
    vm_testcase tests[] = {
        {
            "let one = fn() {let one = 1; one;}; one();",
            (monkey_object_t *) create_monkey_int(1)
        },
        {
            "let oneAndTwo = fn() {let one = 1; let two = 2; one + two;}; oneAndTwo();",
            (monkey_object_t *) create_monkey_int(3)
        },
        {
            "let oneAndTwo = fn() {let one = 1; let two = 2; one + two;}\n"
            "let threeAndFour = fn() {let three = 3; let four = 4; three + four;}\n"
            "oneAndTwo() + threeAndFour();",
            (monkey_object_t *) create_monkey_int(10)
        },
        {
            "let firstFooBar = fn() {let foobar = 50; foobar;}\n"
            "let secondFooBar = fn() { let foobar = 100; foobar;};\n"
            "firstFooBar() + secondFooBar();",
            (monkey_object_t *) create_monkey_int(150)
        },
        {
            "let globalSeed = 50;\n"
            "let minusOne = fn() {\n"
            "  let num = 1;\n"
            "  globalSeed - num;\n"
            "}\n"
            "let minusTwo = fn() {\n"
            "  let num = 2;\n"
            "  globalSeed - num;\n"
            "}\n"
            "minusOne() + minusTwo();",
            (monkey_object_t *) create_monkey_int(97)
        }
    };
    print_test_separator_line();
    printf("Testing function calls with local bindings\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);

}

int
main(int argc, char **argv)
{
    test_integer_aritmetic();
    test_boolean_expressions();
    test_conditionals();
    test_global_let_stmts();
    test_string_expressions();
    test_array_literals();
    test_hash_literals();
    test_index_expresions();
    test_functions_without_arguments();
    test_function_with_return_statement();
    test_functions_without_return_value();
    test_first_class_functions();
    test_calling_functions_with_bindings();
    test_calling_functions_with_bindings_and_arguments();
    test_calling_functions_with_wrong_arguments();
    test_builtin_functions();
    test_closures();
    test_recursive_closures();
    test_recursive_fibonacci();
    return 0;
}
