#include <err.h>
#include <stdlib.h>

#include "compiler.h"
#include "lexer.h"
#include "token.h"
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
        if (error != COMPILER_ERROR_NONE)
            errx(EXIT_FAILURE, "compilation failed for input %s with error %s\n",
                t.input, get_compiler_error(error));
        bytecode_t *bytecode = get_bytecode(compiler);
        vm_t *vm = vm_init(bytecode);
        vm_error_t vm_error = vm_run(vm);
        if (vm_error != VM_ERROR_NONE)
            errx(EXIT_FAILURE, "vm error: %s\n", get_vm_error_desc(vm_error));
        monkey_object_t *top = vm_stack_top(vm);
        test_monkey_object(t.expected, top);
        free_monkey_object(top);
    }
}

static void
test_integer_aritmetic(void)
{
    vm_testcase tests[] = {
        {"1", (monkey_object_t *) create_monkey_int(1)},
        {"2", (monkey_object_t *) create_monkey_int(2)},
        {"1 + 2", (monkey_object_t *) create_monkey_int(3)}
    };

    print_test_separator_line();
    printf("Testing vm for integer arithmetic\n");
    run_vm_tests(3, tests);
}

int
main(int argc, char **argv)
{
    test_integer_aritmetic();
    return 0;
}