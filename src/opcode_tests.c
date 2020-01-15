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
        }
    };
    print_test_separator_line();
    printf("Testing instructions init\n");

    size_t ntests = sizeof(test_cases) / sizeof(test_cases[0]);
    for (size_t i = 0; i < ntests; i++) {
        test t = test_cases[i];
        printf("%s\n", t.desc);
        instructions_t *actual = instruction_init(t.op, t.operands[0]);
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
    instructions_t *ins_array[4] = {
        instruction_init(OPADD),
        instruction_init(OPCONSTANT, 2),
        instruction_init(OPCONSTANT, 65535),
        instruction_init(OPGETLOCAL, 1)
    };

    const char *expected_string = "0000 OPADD\n" \
        "0001 OPCONSTANT 2\n" \
        "0004 OPCONSTANT 65535\n" \
        "0007 OPGETLOCAL 1";
    
    instructions_t *flat_ins = flatten_instructions(4, ins_array);
    char *string = instructions_to_string(flat_ins);
    print_test_separator_line();
    printf("Testing instructions_to_string\n");
    test(strcmp(string, expected_string) == 0, "Expected string [%s], found [%s]\n", expected_string, string);
    free(string);
    instructions_free(flat_ins);
    instructions_free(ins_array[0]);
    instructions_free(ins_array[1]);
    instructions_free(ins_array[2]);
}

int
main(int argc, char **argv)
{
    test_instruction_init();
    test_instructions_string();
    return 0;
}