
#include <err.h>
#include <stdarg.h>

#include "ast.h"
#include "cmonkey_utils.h"
#include "compiler.h"
#include "object.h"
#include "object_test_utils.h"
#include "opcode.h"
#include "parser.h"
#include "test_utils.h"

static cm_array_list *
create_constant_pool(size_t count, ...)
{
    va_list ap;
    va_start(ap, count);
    cm_array_list *list = cm_array_list_init(count, free_monkey_object);
    for (size_t i = 0; i < count; i++) {
        monkey_object_t *obj = (monkey_object_t *) va_arg(ap, monkey_object_t *);
        cm_array_list_add(list, obj);
    }
    return list;
}

typedef struct compiler_test {
    const char *input;
    size_t instructions_count;
    instructions_t *expected_instructions[10];
    cm_array_list *expected_constants;
} compiler_test;

static void
test_instructions(size_t count, uint8_t *expected, uint8_t *actual)
{
    instructions_t expected_ins = {expected, count};
    instructions_t actual_ins = {actual, count};
    for (size_t i = 0; i < count; i++) {
        char *expected_ins_string = instructions_to_string(&expected_ins);
        char *actual_ins_string = instructions_to_string(&actual_ins);
        test(expected[i] == actual[i],
            "Instructions mismatch at byte %zu, expected instructions \"%s\" found \"%s\"\n",
            i, expected_ins_string, actual_ins_string);
        free(expected_ins_string);
        free(actual_ins_string);
    }
}

static void
test_constant_pool(cm_array_list *expected, cm_array_list *actual)
{
    if (expected == NULL) {
        test(actual == NULL, "Expected empty constant pool, found pool with size %zu\n",
            actual->length);
    } else {
        test(expected->length == actual->length,
            "Exepcted constant pool length %zu, found %zu\n",
            expected->length, actual->length);
        for (size_t i = 0; i < expected->length; i++) {
            monkey_object_t *expected_obj = (monkey_object_t *) cm_array_list_get(expected, i);
            monkey_object_t *actual_obj = (monkey_object_t *) cm_array_list_get(actual, i);
            test(expected_obj->type == actual_obj->type,
                "Expected constant type %s at index %zu, found %s\n",
                get_type_name(expected_obj->type), i, get_type_name(actual_obj->type));
            test_monkey_object(actual_obj, expected_obj);
        }
        cm_array_list_free(expected);
    }
}

static void
run_compiler_tests(size_t ntests, compiler_test tests[ntests])
{
    for (size_t i = 0; i < ntests; i++) {
        compiler_test t = tests[i];
        printf("Testing integer expression compilation for %s\n", t.input);
        lexer_t *lexer = lexer_init(t.input);
        parser_t *parser = parser_init(lexer);
        program_t *program = parse_program(parser);
        compiler_t *compiler = compiler_init();
        compiler_error_t e = compile(compiler, (node_t *)program);
        if (e.code != COMPILER_ERROR_NONE)
            errx(EXIT_FAILURE, "Compilation failed for input %s with error %s\n",
                t.input, e.msg);
        bytecode_t *bytecode = get_bytecode(compiler);
        instructions_t *flattened_instructions = flatten_instructions(t.instructions_count, t.expected_instructions);
        char *actual_ins_string = instructions_to_string(bytecode->instructions);
        char *expected_ins_string = instructions_to_string(flattened_instructions);
        test(flattened_instructions->length == bytecode->instructions->length,
            "Exepcted instructions length %zu, found %zu "
            "(expected instructions %s, found %s)\n",
            flattened_instructions->length, bytecode->instructions->length, expected_ins_string, actual_ins_string);
        test_instructions(flattened_instructions->length, flattened_instructions->bytes, bytecode->instructions->bytes);
        test_constant_pool(t.expected_constants, bytecode->constants_pool);
        compiler_free(compiler);
        program_free(program);
        parser_free(parser);
        free(expected_ins_string);
        free(actual_ins_string);
        for (size_t j = 0; j < t.instructions_count; j++)
            instructions_free(t.expected_instructions[j]);
        instructions_free(flattened_instructions);
        bytecode_free(bytecode);
    }
}

static void
test_boolean_expressions(void)
{
    compiler_test tests[] = {
        {
            "true",
            2,
            {
                instruction_init(OPTRUE),
                instruction_init(OPPOP)
            },
            NULL
        },
        {
            "false",
            2,
            {
                instruction_init(OPFALSE),
                instruction_init(OPPOP)
            },
            NULL
        }
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_compiler_tests(ntests, tests);
}

static void
test_integer_arithmetic(void)
{
    compiler_test tests[] = {
        {
            "1 + 2",
            4,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPADD),
                instruction_init(OPPOP)
            },
            create_constant_pool(2, create_monkey_int(1), create_monkey_int(2))
        },
        {
            "1; 2",
            4,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPPOP),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPPOP)
            },
            create_constant_pool(2, create_monkey_int(1), create_monkey_int(2))
        },
        {
            "1 - 2",
            4,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPSUB),
                instruction_init(OPPOP)
            },
            create_constant_pool(2, create_monkey_int(1), create_monkey_int(2))
        },
        {
            "1 * 2",
            4,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPMUL),
                instruction_init(OPPOP)
            },
            create_constant_pool(2, create_monkey_int(1), create_monkey_int(2))
        },
                {
            "2 / 1",
            4,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPDIV),
                instruction_init(OPPOP)
            },
            create_constant_pool(2, create_monkey_int(2), create_monkey_int(1))
        }
    };

    print_test_separator_line();

    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_compiler_tests(ntests, tests);
}

int
main(int argc, char **argv)
{
    test_integer_arithmetic();
    test_boolean_expressions();
}