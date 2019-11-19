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

int
main(int argc, char **argv)
{
    test_integer_aritmetic();
    test_boolean_expressions();
    test_conditionals();
    test_global_let_stmts();
    test_string_expressions();
    return 0;
}