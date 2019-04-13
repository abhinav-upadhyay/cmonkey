#include <stdbool.h>
#include <stdio.h>

#include "cmonkey_utils.h"
#include "evaluator.h"
#include "lexer.h"
#include "parser.h"
#include "object.h"
#include "test_utils.h"

static void
print_test_separator_line(void)
{
    for (int i = 0; i < 100; i++)
        printf("-");
    printf("\n");
}


static monkey_object_t *
test_eval(const char *input)
{
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    monkey_object_t *obj = monkey_eval((node_t *) program);
    program_free(program);
    parser_free(parser);
    return obj;
}

static void
test_boolean_object(monkey_object_t *object, _Bool expected_value)
{
    test(object->type == MONKEY_BOOL, "Expected object of type %s, got %s\n",
        get_type_name(MONKEY_BOOL), get_type_name(object->type));
    monkey_bool_t *bool_obj = (monkey_bool_t *) object;
    test(bool_obj->value == expected_value, "Expected bool value %s, got %s\n",
        bool_to_string(expected_value), bool_to_string(bool_obj->value));
}

static void
test_integer_object(monkey_object_t *object, long expected_value)
{
    test(object->type == MONKEY_INT, "Expected object of type %s, got %s\n",
        get_type_name(MONKEY_INT), get_type_name(object->type));
    
    monkey_int_t *int_obj = (monkey_int_t *) object;
    test(int_obj->value == expected_value,
        "Expected integer object value to be %ld, found %ld\n",
        expected_value, int_obj->value);
    free_monkey_object(object);
}

static void
test_eval_integer_expression(void)
{
    typedef struct {
        const char *input;
        long expected_value;
    } test_input;

    test_input tests[] = {
        {"5", 5},
        {"10", 10},
        {"0", 0},
        {"-5", -5},
        {"-10", -10},
        {"5 + 5 + 5 + 5 - 10", 10},
        {"2 * 2 * 2 * 2 * 2", 32},
        {"-50 + 100 - 50", 0},
        {"5 * 2 + 10", 20},
        {"5 + 2 * 10", 25},
        {"20 + 2 * -10", 0},
        {"50 / 2 * 2 + 10", 60},
        {"2 * (5 + 10)", 30},
        {"3 * 3 * 3 + 10", 37},
        {"3 * (3 * 3) + 10", 37},
        {"(5 + 10 * 2 + 15 / 3) * 2 + -10", 50}
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing eval for integer expression: %s\n", test.input);
        monkey_object_t *obj = test_eval(test.input);
        test_integer_object(obj, test.expected_value);
    }
    printf("integer expression eval test passed\n");
}

static void
test_eval_bool_expression(void)
{
    typedef struct {
        const char *input;
        _Bool expected_value;
    } test_input;

    test_input tests[] = {
        {"true", true},
        {"false", false},
        {"1 < 2", true},
        {"1 > 2", false},
        {"1 < 1", false},
        {"1 > 1", false},
        {"1 == 1", true},
        {"1 != 1", false},
        {"1 == 2", false},
        {"1 != 2", true}
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing eval for boolean expression: %s\n", test.input);
        monkey_object_t *obj = test_eval(test.input);
        test_boolean_object(obj, test.expected_value);
        free_monkey_object(obj);
    }
    printf("boolean expression eval test passed\n");
}

static void
test_bang_operator(void)
{
    typedef struct {
        const char *input;
        _Bool expected;
    } test_input;

    test_input tests[] = {
        {"!true", false},
        {"!false", true},
        {"!5", false},
        {"!!true", true},
        {"!!false", false},
        {"!!5", true},
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing bang operator for expression: %s\n", test.input);
        monkey_object_t *obj = test_eval(test.input);
        test_boolean_object(obj, test.expected);
        free_monkey_object(obj);
    }
}

int
main(int argc, char **argv)
{
    test_eval_integer_expression();
    test_eval_bool_expression();
    test_bang_operator();
    return 0;
}
