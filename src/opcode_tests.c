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

#include <stdbool.h>
#include <string.h>
#include "cmonkey_utils.h"
#include "opcode.h"
#include "test_utils.h"

static void
test_instructions(size_t size, uint8_t *expected, uint8_t *actual)
{
    for (size_t i = 0; i < size; i++)
        test(expected[i] == actual[i], "Expected byte %u, found %u\n", expected[i], actual[i]);
}

static void
test_instruction_init(void)
{
    typedef struct {
        const char *desc;
        opcode_t op;
        size_t operands[MAX_OPERANDS];
        size_t expected_instructions_len;
        uint8_t *expected_instructions;
    } test;

    test test_cases[] = {
        {
            "Testing OPCONSTANT 65534",
            OPCONSTANT, {(size_t) 65534},
            3,
            create_uint8_array(4, OPCONSTANT, 255, 254)
        },
        {
            "Test OPADD",
            OPADD,
            {},
            1,
            create_uint8_array(1, OPADD)
        },
        {
            "Test OPSETLOCAL 255",
            OPSETLOCAL, {(size_t) 255},
            2,
            create_uint8_array(2, OPSETLOCAL, 255)
        },
        {
            "Test OPCLOSURE 65534 255",
            OPCLOSURE, {(size_t) 65534, (size_t) 255},
            4,
            create_uint8_array(4, OPCLOSURE, 255, 254, 255)
        }
    };
    print_test_separator_line();
    printf("Testing instructions init\n");

    size_t ntests = sizeof(test_cases) / sizeof(test_cases[0]);
    for (size_t i = 0; i < ntests; i++) {
        test t = test_cases[i];
        printf("%s\n", t.desc);
        instructions_t *actual;
        if (t.op != OPCLOSURE)
            actual = instruction_init(t.op, t.operands[0]);
        else
            actual = instruction_init(t.op, t.operands[0], t.operands[1]);
        size_t actual_len = actual->length;
        size_t expected_len = t.expected_instructions_len;
        test(actual_len == expected_len, "Expected instruction length %zu, found %zu\n", expected_len, actual_len);
        test_instructions(actual_len, t.expected_instructions, actual->bytes);
        instructions_free(actual);
        free(t.expected_instructions);
    }
}

static void
test_instructions_string(void)
{
    instructions_t *ins_array[5] = {
        instruction_init(OPADD),
        instruction_init(OPCONSTANT, 2),
        instruction_init(OPCONSTANT, 65535),
        instruction_init(OPGETLOCAL, 1),
        instruction_init(OPCLOSURE, 65535, 255)
    };

    const char *expected_string = "0000 OPADD\n" \
        "0001 OPCONSTANT 2\n" \
        "0004 OPCONSTANT 65535\n" \
        "0007 OPGETLOCAL 1\n" \
        "0009 OPCLOSURE 65535 255";
    
    instructions_t *flat_ins = flatten_instructions(5, ins_array);
    char *string = instructions_to_string(flat_ins);
    print_test_separator_line();
    printf("Testing instructions_to_string\n");
    test(strcmp(string, expected_string) == 0, "Expected string [%s], found [%s]\n", expected_string, string);
    free(string);
    instructions_free(flat_ins);
    instructions_free(ins_array[0]);
    instructions_free(ins_array[1]);
    instructions_free(ins_array[2]);
    instructions_free(ins_array[3]);
    instructions_free(ins_array[4]);
}

int
main(int argc, char **argv)
{
    test_instruction_init();
    test_instructions_string();
    return 0;
}