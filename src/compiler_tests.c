/*-
 * Copyright (c) 2019 Abhinav Upadhyay <er.abhinav.upadhyay@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
    instructions_t *expected_instructions[32];
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
        printf("Testing compilation for %s\n", t.input);
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
test_compiler_scopes(void)
{
    printf("Testing compiler scopes\n");
    print_test_separator_line();
    instructions_t *ins;
    compiler_t *compiler = compiler_init();
    symbol_table_t *global_symbol_table = compiler->symbol_table;
    opcode_definition_t op_def;
    test(compiler->scope_index == 0,
        "Expected scope index to be 0, found %zu\n", compiler->scope_index);
    emit(compiler, OPMUL);
    compiler_enter_scope(compiler);
    test(compiler->scope_index == 1,
        "Expected scope index to be 1, found %zu\n", compiler->scope_index);
    emit(compiler, OPSUB);
    compilation_scope_t *scope = get_top_scope(compiler);
    test(scope != NULL, "scope NULL at index %zu\n", compiler->scope_index);
    test(scope->instructions->length == 1,
        "Expected instructions length 1 at scope index %zu, found %zu\n",
        compiler->scope_index, scope->instructions->length);
    op_def = opcode_definition_lookup(scope->last_instruction.opcode);
    test(scope->last_instruction.opcode == OPSUB,
        "Expected last opcode OPSUB, found %s\n", op_def.name);
    test(compiler->symbol_table->outer == global_symbol_table, "compiler did not enclose symbol table\n");
    ins = compiler_leave_scope(compiler);
    test(compiler->scope_index == 0, "Expected scope index 0, found %zu\n",
        compiler->scope_index);
    test(compiler->symbol_table == global_symbol_table, "compiler did not restore symbol table\n");
    test(compiler->symbol_table->outer == NULL, "compiler modified global symbol table incorrectly\n");
    instructions_free(ins);
    emit(compiler, OPADD);
    scope = get_top_scope(compiler);
    test(scope->instructions->length == 2, "Expected scope length 2, found %zu\n",
        scope->instructions->length);
    op_def = opcode_definition_lookup(scope->last_instruction.opcode);
    test(scope->last_instruction.opcode == OPADD,
        "Expected last opcode OPADD, found %s\n", op_def.name);
    op_def = opcode_definition_lookup(scope->prev_instruction.opcode);
    test(scope->prev_instruction.opcode == OPMUL,
        "Expected previous instruction OPMUL, found %s\n", op_def.name);
    compiler_free(compiler);
}

static void
test_conditionals(void)
{
    compiler_test tests[] = {
        {
            "if (true) {10}; 3333;",
            8,
            {
                instruction_init(OPTRUE),
                instruction_init(OPJMPFALSE, 10),
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPJMP, 11),
                instruction_init(OPNULL),
                instruction_init(OPPOP),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPPOP)
            },
            create_constant_pool(2, (monkey_object_t *) create_monkey_int(10),
                (monkey_object_t *) create_monkey_int(3333))
        },
        {
            "if (true) {10} else {20}; 3333;",
            8,
            {
                instruction_init(OPTRUE),
                instruction_init(OPJMPFALSE, 10),
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPJMP, 13),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPPOP),
                instruction_init(OPCONSTANT, 2),
                instruction_init(OPPOP)
            },
            create_constant_pool(3, (monkey_object_t *) create_monkey_int(10),
                (monkey_object_t *) create_monkey_int(20),
                (monkey_object_t *) create_monkey_int(3333))
        }
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_compiler_tests(ntests, tests);
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
        },
        {
            "1 > 2",
            4,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPGREATERTHAN),
                instruction_init(OPPOP)
            },
            create_constant_pool(2, create_monkey_int(1), create_monkey_int(2))
        },
        {
            "1 < 2",
            4,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPGREATERTHAN),
                instruction_init(OPPOP)
            },
            create_constant_pool(2, create_monkey_int(2), create_monkey_int(1))
        },
        {
            "1 == 2",
            4,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPEQUAL),
                instruction_init(OPPOP)
            },
            create_constant_pool(2, create_monkey_int(1), create_monkey_int(2))
        },
        {
            "1 != 2",
            4,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPNOTEQUAL),
                instruction_init(OPPOP)
            },
            create_constant_pool(2, create_monkey_int(1), create_monkey_int(2))
        },
        {
            "true == true",
            4,
            {
                instruction_init(OPTRUE),
                instruction_init(OPTRUE),
                instruction_init(OPEQUAL),
                instruction_init(OPPOP)
            },
            NULL
        },
        {
            "true != false",
            4,
            {
                instruction_init(OPTRUE),
                instruction_init(OPFALSE),
                instruction_init(OPNOTEQUAL),
                instruction_init(OPPOP)
            },
            NULL
        },
        {
            "!true",
            3,
            {
                instruction_init(OPTRUE),
                instruction_init(OPBANG),
                instruction_init(OPPOP)
            },
            NULL
        }
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_compiler_tests(ntests, tests);
}

static void
test_global_let_statements(void)
{
    compiler_test tests[] = {
        {
            "let one = 1; let two = 2;",
            4,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPSETGLOBAL, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPSETGLOBAL, 1)
            },
            create_constant_pool(2, create_monkey_int(1), create_monkey_int(2))
        },
        {
            "let one = 1; one;",
            4,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPSETGLOBAL, 0),
                instruction_init(OPGETGLOBAL, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(1, create_monkey_int(1))
        }

    };
    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_compiler_tests(ntests, tests);
}

static void
test_string_expressions(void)
{
    compiler_test tests[] = {
        {
            "\"monkey\"",
            2,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(1, create_monkey_string("monkey", 6))
        },
        {
            "\"mon\" + \"key\"",
            4,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPADD),
                instruction_init(OPPOP)
            },
            create_constant_pool(2, create_monkey_string("mon", 3), create_monkey_string("key", 3))
        }
    };
    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_compiler_tests(ntests, tests);
}

static void
test_hash_literals(void)
{
    print_test_separator_line();
    printf("Testing hash literals\n");

    compiler_test tests[] = {
        {
            "{}",
            2,
            {
                instruction_init(OPHASH, 0),
                instruction_init(OPPOP)
            },
            NULL
        },
        {
            "{1: 2, 3: 4, 5: 6}",
            8,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPCONSTANT, 2),
                instruction_init(OPCONSTANT, 3),
                instruction_init(OPCONSTANT, 4),
                instruction_init(OPCONSTANT, 5),
                instruction_init(OPHASH, 6),
                instruction_init(OPPOP)
            },
            create_constant_pool(6,
                (monkey_object_t *) create_monkey_int(1),
                (monkey_object_t *) create_monkey_int(2),
                (monkey_object_t *) create_monkey_int(3),
                (monkey_object_t *) create_monkey_int(4),
                (monkey_object_t *) create_monkey_int(5),
                (monkey_object_t *) create_monkey_int(6))
        },
        {
            "{1: 2 + 3, 4: 5 * 6}",
            10,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPCONSTANT, 2),
                instruction_init(OPADD),
                instruction_init(OPCONSTANT, 3),
                instruction_init(OPCONSTANT, 4),
                instruction_init(OPCONSTANT, 5),
                instruction_init(OPMUL),
                instruction_init(OPHASH, 4),
                instruction_init(OPPOP)
            },
            create_constant_pool(6,
                (monkey_object_t *) create_monkey_int(1),
                (monkey_object_t *) create_monkey_int(2),
                (monkey_object_t *) create_monkey_int(3),
                (monkey_object_t *) create_monkey_int(4),
                (monkey_object_t *) create_monkey_int(5),
                (monkey_object_t *) create_monkey_int(6))
        }
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_compiler_tests(ntests, tests);
}

static void
test_array_literals(void)
{
    print_test_separator_line();
    printf("Testing array literals\n");

    compiler_test tests[] = {
        {
            "[]",
            2,
            {
                instruction_init(OPARRAY, 0),
                instruction_init(OPPOP)
            },
            NULL
        },
        {
            "[1, 2, 3]",
            5,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPCONSTANT, 2),
                instruction_init(OPARRAY, 3),
                instruction_init(OPPOP)
            },
            create_constant_pool(3,
                (monkey_object_t *) create_monkey_int(1),
                (monkey_object_t *) create_monkey_int(2),
                (monkey_object_t *) create_monkey_int(3))
        },
        {
            "[1 + 2, 3 - 4, 5 * 6]",
            11,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPADD),
                instruction_init(OPCONSTANT, 2),
                instruction_init(OPCONSTANT, 3),
                instruction_init(OPSUB),
                instruction_init(OPCONSTANT, 4),
                instruction_init(OPCONSTANT, 5),
                instruction_init(OPMUL),
                instruction_init(OPARRAY, 3),
                instruction_init(OPPOP)
            },
            create_constant_pool(6,
                (monkey_object_t *) create_monkey_int(1),
                (monkey_object_t *) create_monkey_int(2),
                (monkey_object_t *) create_monkey_int(3),
                (monkey_object_t *) create_monkey_int(4),
                (monkey_object_t *) create_monkey_int(5),
                (monkey_object_t *) create_monkey_int(6))
        }
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_compiler_tests(ntests, tests);
}

static void
test_index_expressions(void)
{
    compiler_test tests[] = {
        {
            "[1, 2, 3][1 + 2]",
            9,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPCONSTANT, 2),
                instruction_init(OPARRAY, 3),
                instruction_init(OPCONSTANT, 3),
                instruction_init(OPCONSTANT, 4),
                instruction_init(OPADD),
                instruction_init(OPINDEX),
                instruction_init(OPPOP)
            },
            create_constant_pool(5,
                (monkey_object_t *) create_monkey_int(1),
                (monkey_object_t *) create_monkey_int(2),
                (monkey_object_t *) create_monkey_int(3),
                (monkey_object_t *) create_monkey_int(1),
                (monkey_object_t *) create_monkey_int(2))
        },
        {
            "{1: 2}[2 - 1]",
            8,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPHASH, 2),
                instruction_init(OPCONSTANT, 2),
                instruction_init(OPCONSTANT, 3),
                instruction_init(OPSUB),
                instruction_init(OPINDEX),
                instruction_init(OPPOP)
            },
            create_constant_pool(4,
                (monkey_object_t *) create_monkey_int(1),
                (monkey_object_t *) create_monkey_int(2),
                (monkey_object_t *) create_monkey_int(2),
                (monkey_object_t *) create_monkey_int(1))
        }
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_compiler_tests(ntests, tests);

}

static instructions_t *
create_compiled_fn_instructions(size_t nins, ...)
{
    va_list ap;
    va_start(ap, nins);
    instructions_t *retval = NULL;
    for (size_t i = 0; i < nins; i++) {
        instructions_t *ins = (instructions_t *) va_arg(ap, instructions_t *);
        if (retval == NULL)
            retval = ins;
        else {
            concat_instructions(retval, ins);
            instructions_free(ins);
        }
    }
    return retval;
}

static void
test_recursive_functions(void)
{
    compiler_test tests[] = {
        {
            "let countDown = fn(x) { countDown(x - 1);};\n"
            "countDown(1)",
            6,
            {
                instruction_init(OPCLOSURE, 1, 0),
                instruction_init(OPSETGLOBAL, 0),
                instruction_init(OPGETGLOBAL, 0),
                instruction_init(OPCONSTANT, 2),
                instruction_init(OPCALL, 1),
                instruction_init(OPPOP)
            },
            create_constant_pool(3,
                (monkey_object_t *) create_monkey_int(1),
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(6,
                    instruction_init(OPCURRENTCLOSURE),
                    instruction_init(OPGETLOCAL, 0),
                    instruction_init(OPCONSTANT, 0),
                    instruction_init(OPSUB),
                    instruction_init(OPCALL, 1),
                    instruction_init(OPRETURNVALUE)), 0, 1),
                (monkey_object_t *) create_monkey_int(1))
        },
        {
            "let wrapper = fn() {\n"
            "   let countDown = fn(x) {\n"
            "       countDown(x - 1);\n"
            "   };\n"
            "   countDown(1);\n"
            "   }\n"
            "wrapper();",
            5,
            {
                instruction_init(OPCLOSURE, 3, 0),
                instruction_init(OPSETGLOBAL, 0),
                instruction_init(OPGETGLOBAL, 0),
                instruction_init(OPCALL, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(4,
                (monkey_object_t *) create_monkey_int(1),
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(6,
                    instruction_init(OPCURRENTCLOSURE),
                    instruction_init(OPGETLOCAL, 0),
                    instruction_init(OPCONSTANT, 0),
                    instruction_init(OPSUB),
                    instruction_init(OPCALL, 1),
                    instruction_init(OPRETURNVALUE)), 0, 1),
                (monkey_object_t *) create_monkey_int(1),
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(6,
                    instruction_init(OPCLOSURE, 1, 0),
                    instruction_init(OPSETLOCAL, 0),
                    instruction_init(OPGETLOCAL, 0),
                    instruction_init(OPCONSTANT, 2),
                    instruction_init(OPCALL, 1),
                    instruction_init(OPRETURNVALUE)), 1, 0))
        }
    };
    print_test_separator_line();
    printf("Testing recursive function calls\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_compiler_tests(ntests, tests);

}

static void
test_function_calls(void)
{
    compiler_test tests[] = {
        {
            "fn() {24}();",
            3,
            {
                instruction_init(OPCLOSURE, 1, 0),
                instruction_init(OPCALL, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(2,
                (monkey_object_t *) create_monkey_int(24),
                (monkey_object_t *) create_monkey_compiled_fn(
                    create_compiled_fn_instructions(2,
                    instruction_init(OPCONSTANT, 0),
                    instruction_init(OPRETURNVALUE)), 0, 0))
        },
        {
            "let noArg = fn() {24}; noArg();",
            5,
            {
                instruction_init(OPCLOSURE, 1, 0),
                instruction_init(OPSETGLOBAL, 0),
                instruction_init(OPGETGLOBAL, 0),
                instruction_init(OPCALL, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(2,
                (monkey_object_t *) create_monkey_int(24),
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(2,
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPRETURNVALUE)), 0, 0))
        },
        {
            "let oneArg = fn(a) {a};\n oneArg(24);",
            6,
            {
                instruction_init(OPCLOSURE, 0, 0),
                instruction_init(OPSETGLOBAL, 0),
                instruction_init(OPGETGLOBAL, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPCALL, 1),
                instruction_init(OPPOP)
            },
            create_constant_pool(2,
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(2,
                    instruction_init(OPGETLOCAL, 0),
                    instruction_init(OPRETURNVALUE)), 0, 1),
                (monkey_object_t *) create_monkey_int(24))
        },
        {
            "let manyArg = fn(a, b, c) {a; b; c;};\n manyArg(24, 25, 26);",
            8,
            {
                instruction_init(OPCLOSURE, 0, 0),
                instruction_init(OPSETGLOBAL, 0),
                instruction_init(OPGETGLOBAL, 0),
                instruction_init(OPCONSTANT, 1),
                instruction_init(OPCONSTANT, 2),
                instruction_init(OPCONSTANT, 3),
                instruction_init(OPCALL, 3),
                instruction_init(OPPOP)
            },
            create_constant_pool(4,
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(6,
                    instruction_init(OPGETLOCAL, 0),
                    instruction_init(OPPOP),
                    instruction_init(OPGETLOCAL, 1),
                    instruction_init(OPPOP),
                    instruction_init(OPGETLOCAL, 2),
                    instruction_init(OPRETURNVALUE)), 0, 3),
                (monkey_object_t *) create_monkey_int(24),
                (monkey_object_t *) create_monkey_int(25),
                (monkey_object_t *) create_monkey_int(26))

        }
    };
    print_test_separator_line();
    printf("Testing function calls\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_compiler_tests(ntests, tests);
}

static void
test_closures(void)
{
    compiler_test tests[] = {
        {
            "fn(a) {\n"
            "  fn(b) {\n"
            "    a + b\n"
            "  }\n"
            "};\n",
            2,
            {
                instruction_init(OPCLOSURE, 1, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(2,
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(4,
                instruction_init(OPGETFREE, 0),
                instruction_init(OPGETLOCAL, 0),
                instruction_init(OPADD),
                instruction_init(OPRETURNVALUE)), 0, 1),
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(3,
                instruction_init(OPGETLOCAL, 0),
                instruction_init(OPCLOSURE, 0, 1),
                instruction_init(OPRETURNVALUE)), 0, 1))
        },
        {
            "fn(a) {\n"
            "   fn(b) {\n"
            "       fn(c) {\n"
            "           a + b + c\n"
            "       }\n"
            "   }\n"
            "}",
            2,
            {
                instruction_init(OPCLOSURE, 2, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(3,
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(6,
                    instruction_init(OPGETFREE, 0),
                    instruction_init(OPGETFREE, 1),
                    instruction_init(OPADD),
                    instruction_init(OPGETLOCAL, 0),
                    instruction_init(OPADD),
                    instruction_init(OPRETURNVALUE)), 0, 1),
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(4,
                    instruction_init(OPGETFREE, 0),
                    instruction_init(OPGETLOCAL, 0),
                    instruction_init(OPCLOSURE, 0, 2),
                    instruction_init(OPRETURNVALUE)), 0, 1),
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(3,
                    instruction_init(OPGETLOCAL, 0),
                    instruction_init(OPCLOSURE, 1, 1),
                    instruction_init(OPRETURNVALUE)), 0, 1))
        },
        {
            "let global = 55;\n"
            "fn() {\n"
            "   let a = 66;\n"
            "   fn() {\n"
            "       let b = 77;\n"
            "       fn() {\n"
            "           let c = 88;\n"
            "           global + a + b + c;\n"
            "       }\n"
            "   }\n"
            "}",
            4,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPSETGLOBAL, 0),
                instruction_init(OPCLOSURE, 6, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(7,
                (monkey_object_t *) create_monkey_int(55),
                (monkey_object_t *) create_monkey_int(66),
                (monkey_object_t *) create_monkey_int(77),
                (monkey_object_t *) create_monkey_int(88),
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(10,
                    instruction_init(OPCONSTANT, 3),
                    instruction_init(OPSETLOCAL, 0),
                    instruction_init(OPGETGLOBAL, 0),
                    instruction_init(OPGETFREE, 0),
                    instruction_init(OPADD),
                    instruction_init(OPGETFREE, 1),
                    instruction_init(OPADD),
                    instruction_init(OPGETLOCAL, 0),
                    instruction_init(OPADD),
                    instruction_init(OPRETURNVALUE)), 1, 0),
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(6,
                    instruction_init(OPCONSTANT, 2),
                    instruction_init(OPSETLOCAL, 0),
                    instruction_init(OPGETFREE, 0),
                    instruction_init(OPGETLOCAL, 0),
                    instruction_init(OPCLOSURE, 4, 2),
                    instruction_init(OPRETURNVALUE)), 1, 0),
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(5,
                    instruction_init(OPCONSTANT, 1),
                    instruction_init(OPSETLOCAL, 0),
                    instruction_init(OPGETLOCAL, 0),
                    instruction_init(OPCLOSURE, 5, 1),
                    instruction_init(OPRETURNVALUE)), 1, 0))
        }
    };
    print_test_separator_line();
    printf("Testing closures compilation\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_compiler_tests(ntests, tests);
}

static void
test_functions(void)
{
    compiler_test tests[] = {
        {
            "fn() {return 5 + 10}",
            2,
            {
                instruction_init(OPCLOSURE, 2, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(3,
                (monkey_object_t *) create_monkey_int(5),
                (monkey_object_t *) create_monkey_int(10),
                (monkey_object_t *) create_monkey_compiled_fn(
                    create_compiled_fn_instructions(4,
                        instruction_init(OPCONSTANT, 0),
                        instruction_init(OPCONSTANT, 1),
                        instruction_init(OPADD),
                        instruction_init(OPRETURNVALUE)), 0, 0))
        },
        {
            "fn() {5 + 10}",
            2,
            {
                instruction_init(OPCLOSURE, 2, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(3,
                (monkey_object_t *) create_monkey_int(5),
                (monkey_object_t *) create_monkey_int(10),
                (monkey_object_t *) create_monkey_compiled_fn(
                    create_compiled_fn_instructions(4,
                        instruction_init(OPCONSTANT, 0),
                        instruction_init(OPCONSTANT, 1),
                        instruction_init(OPADD),
                        instruction_init(OPRETURNVALUE)), 0, 0))
        },
        {
            "fn() {1; 2;}",
            2,
            {
                instruction_init(OPCLOSURE, 2, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(3,
                (monkey_object_t *) create_monkey_int(1),
                (monkey_object_t *) create_monkey_int(2),
                (monkey_object_t *) create_monkey_compiled_fn(
                    create_compiled_fn_instructions(4,
                        instruction_init(OPCONSTANT, 0),
                        instruction_init(OPPOP),
                        instruction_init(OPCONSTANT, 1),
                        instruction_init(OPRETURNVALUE)), 0, 0))
        },
        {
            "fn() {}",
            2,
            {
                instruction_init(OPCLOSURE, 0, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(1,
                create_monkey_compiled_fn(create_compiled_fn_instructions(1, instruction_init(OPRETURN)), 0, 0))
        }
    };
    print_test_separator_line();
    printf("Testing function compilation\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_compiler_tests(ntests, tests);
}

static void
test_let_statement_scope(void)
{
    compiler_test tests[] = {
        {
            "let num = 55;\n"
            "fn() { num };",
            4,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPSETGLOBAL, 0),
                instruction_init(OPCLOSURE, 1, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(2,
                (monkey_object_t *) create_monkey_int(55),
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(2,
                    instruction_init(OPGETGLOBAL, 0),
                    instruction_init(OPRETURNVALUE)), 0, 0))
        },
        {
            "fn() {\n"
            "  let num = 55;\n"
            "  num;\n"
            "}",
            2,
            {
                instruction_init(OPCLOSURE, 1, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(2,
                (monkey_object_t *) create_monkey_int(55),
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(4,
                    instruction_init(OPCONSTANT, 0),
                    instruction_init(OPSETLOCAL, 0),
                    instruction_init(OPGETLOCAL, 0),
                    instruction_init(OPRETURNVALUE)), 1, 0))
        },
        {
            "fn() {\n"
            "  let a = 55;\n"
            "  let b = 77;\n"
            "  a + b;\n"
            "}",
            2,
            {
                instruction_init(OPCLOSURE, 2, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(3,
                (monkey_object_t *) create_monkey_int(55),
                (monkey_object_t *) create_monkey_int(77),
                (monkey_object_t *) create_monkey_compiled_fn(create_compiled_fn_instructions(8,
                    instruction_init(OPCONSTANT, 0),
                    instruction_init(OPSETLOCAL, 0),
                    instruction_init(OPCONSTANT, 1),
                    instruction_init(OPSETLOCAL, 1),
                    instruction_init(OPGETLOCAL, 0),
                    instruction_init(OPGETLOCAL, 1),
                    instruction_init(OPADD),
                    instruction_init(OPRETURNVALUE)), 2, 0))
        }
    };
    print_test_separator_line();
    printf("Testing let statements with scopes\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    run_compiler_tests(ntests, tests);
}

static void
test_builtins(void)
{
    compiler_test tests[] = {
        {
            "len([]);\n"
            "push([], 1);",
            9,
            {
                instruction_init(OPGETBUILTIN, 0),
                instruction_init(OPARRAY, 0),
                instruction_init(OPCALL, 1),
                instruction_init(OPPOP),
                instruction_init(OPGETBUILTIN, 5),
                instruction_init(OPARRAY, 0),
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPCALL, 2),
                instruction_init(OPPOP)
            },
            create_constant_pool(1, (monkey_object_t *) create_monkey_int(1))
        },
        {
            "fn() {len([]);};",
            2,
            {
                instruction_init(OPCLOSURE, 0, 0),
                instruction_init(OPPOP)
            },
            create_constant_pool(1,
                (monkey_object_t *) create_monkey_compiled_fn(
                    create_compiled_fn_instructions(4,
                    instruction_init(OPGETBUILTIN, 0),
                    instruction_init(OPARRAY, 0),
                    instruction_init(OPCALL, 1),
                    instruction_init(OPRETURNVALUE)), 0, 0))
        }
    };

    print_test_separator_line();
    printf("Tesing builtin functions compilation\n");
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
        },
        {
            "-1",
            3,
            {
                instruction_init(OPCONSTANT, 0),
                instruction_init(OPMINUS),
                instruction_init(OPPOP)
            },
            create_constant_pool(1, create_monkey_int(1))
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
    test_conditionals();
    test_global_let_statements();
    test_string_expressions();
    test_array_literals();
    test_hash_literals();
    test_index_expressions();
    test_compiler_scopes();
    test_functions();
    test_function_calls();
    test_let_statement_scope();
    test_builtins();
    test_closures();
    test_recursive_functions();
}