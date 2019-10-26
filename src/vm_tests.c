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
        if (vm_error != VM_ERROR_NONE)
            errx(EXIT_FAILURE, "vm error: %s\n", get_vm_error_desc(vm_error));
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
        {"(1 > 2) == true", (monkey_object_t *) create_monkey_bool(false)}
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
        {"5 * (2 + 10)", (monkey_object_t *) create_monkey_int(60)}
    };

    print_test_separator_line();
    printf("Testing vm for integer arithmetic\n");
    size_t ntests = sizeof(tests)/ sizeof(tests[0]);
    run_vm_tests(ntests, tests);
    for (size_t i = 0; i < ntests; i++)
        free_monkey_object(tests[i].expected);
}

int
main(int argc, char **argv)
{
    test_integer_aritmetic();
    test_boolean_expressions();
    return 0;
}