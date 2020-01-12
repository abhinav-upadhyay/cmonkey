#include <err.h>
#include <stdlib.h>

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
    return 0;
}