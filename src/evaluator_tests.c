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
    free(object);
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
    free(object);
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
        {"10", 10}
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
        {"false", false}
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing eval for boolean expression: %s\n", test.input);
        monkey_object_t *obj = test_eval(test.input);
        test_boolean_object(obj, test.expected_value);
    }
    printf("boolean expression eval test passed\n");
}

int
main(int argc, char **argv)
{
    test_eval_integer_expression();
    test_eval_bool_expression();
    return 0;
}
