#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "cmonkey_utils.h"
#include "environment.h"
#include "evaluator.h"
#include "lexer.h"
#include "parser.h"
#include "object.h"
#include "test_utils.h"

static monkey_object_t *
test_eval(const char *input, environment_t *env)
{
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    monkey_object_t *obj = monkey_eval((node_t *) program, env);
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
test_null_object(monkey_object_t *object)
{
    test(object->type == MONKEY_NULL, "Expected a MONKEY_NULL object, found %s\n",
        get_type_name(object->type));
    test(object == (monkey_object_t *) create_monkey_null(), "object is not NULL\n");
}

static void
test_eval_integer_expression(void)
{
    environment_t *env;
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
        env = create_env();
        monkey_object_t *obj = test_eval(test.input, env);
        test_integer_object(obj, test.expected_value);
        env_free(env);
    }
    printf("integer expression eval test passed\n");
}

static void
test_eval_bool_expression(void)
{
    environment_t *env;
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
        {"1 != 2", true},
        {"true == true", true},
        {"true == false", false},
        {"false == false", true},
        {"false == true", false},
        {"true != true", false},
        {"true != false", true},
        {"false != false", false},
        {"false != true", true},
        {"(1 < 2) == true", true},
        {"(1 < 2) == false", false},
        {"(1 > 2) == true", false},
        {"(1 > 2) == false", true}
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing eval for boolean expression: %s\n", test.input);
        env = create_env();
        monkey_object_t *obj = test_eval(test.input, env);
        test_boolean_object(obj, test.expected_value);
        free_monkey_object(obj);
        env_free(env);
    }
    printf("boolean expression eval test passed\n");
}

static void
test_bang_operator(void)
{
    environment_t *env;

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
        env = create_env();
        monkey_object_t *obj = test_eval(test.input, env);
        test_boolean_object(obj, test.expected);
        free_monkey_object(obj);
        env_free(env);
    }
}

static void
_test_monkey_object(monkey_object_t *obj, monkey_object_t *expected)
{
    test(obj->type == expected->type, "Expected object of type %s, got %s\n",
        get_type_name(expected->type), get_type_name(obj->type));
    if (expected->type == MONKEY_INT)
        test_integer_object(obj, ((monkey_int_t *) expected)->value);
    else if (expected->type == MONKEY_BOOL)
        test_boolean_object(obj, ((monkey_bool_t *) expected)->value);
}

static void
test_if_else_expressions(void)
{
    environment_t *env;
    typedef struct {
        const char *input;
        monkey_object_t *expected;
    } test_input;

    test_input tests[] = {
        {"if (true) { 10 }", (monkey_object_t *) create_monkey_int(10)},
        {"if (false) { 10 }", (monkey_object_t *) create_monkey_null()},
        {"if (1) { 10 }", (monkey_object_t *) create_monkey_int(10)},
        {"if (1 < 2) { 10 }", (monkey_object_t *) create_monkey_int(10)},
        {"if (1 > 2) { 10 }", (monkey_object_t *) create_monkey_null()},
        {"if (1 > 2) { 10 } else { 20 }", (monkey_object_t *) create_monkey_int(20)},
        {"if (1 < 2) { 10 } else { 20 }", (monkey_object_t *) create_monkey_int(10)}
    };
    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing if else expression evaluation for \"%s\"\n", test.input);
        env = create_env();
        monkey_object_t *evaluated = test_eval(test.input, env);
        if (test.expected->type == MONKEY_INT) {
            monkey_int_t *expected_int = (monkey_int_t *) test.expected;
            test_integer_object(evaluated, expected_int->value);
        } else
            test_null_object(evaluated);
        free_monkey_object(test.expected);
        env_free(env);
    }
}

static void
test_return_statements(void)
{
    environment_t *env;
    typedef struct {
        const char *input;
        monkey_object_t *expected;
    } test_input;

    test_input tests[] = {
        {"return 10;", (monkey_object_t *) create_monkey_int(10)},
        {"return 10; 9;", (monkey_object_t *) create_monkey_int(10)},
        {"return 2 * 5; 9;", (monkey_object_t *) create_monkey_int(10)},
        {"9; return 2 * 5; 9", (monkey_object_t *) create_monkey_int(10)},
        {"if (10 > 1) {\n"\
        "   if (10 > 1) {\n"\
                "return 10;\n"\
            "}\n"\
            "return 1;\n"\
        "}\n", (monkey_object_t *) create_monkey_int(10)}
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing return statement evaluation for \"%s\"\n", test.input);
        env = create_env();
        monkey_object_t *evaluated = test_eval(test.input, env);
        _test_monkey_object(evaluated, test.expected);
        free_monkey_object(test.expected);
        env_free(env);
    }
}

static void
test_error_handling(void)
{
    typedef struct {
        const char *input;
        const char *message;
    } test_input;
    test_input tests[] = {
        {
            "5 + true;",
            "type mismatch: INTEGER + BOOLEAN"
        },
        {
            "5 + true; 5;",
            "type mismatch: INTEGER + BOOLEAN"
        },
        {
            "-true",
            "unknown operator: -BOOLEAN"
        },
        {
            "true + false;",
            "unknown operator: BOOLEAN + BOOLEAN"
        },
        {
            "5; true + false; 5",
            "unknown operator: BOOLEAN + BOOLEAN"
        },
        {
            "if (10 > 1) { true + false;}",
            "unknown operator: BOOLEAN + BOOLEAN"
        },
        {
            "if (10 > 1) {\n"\
            "   if (10 > 1) {\n"\
            "       return true + false;\n"\
            "   }\n"\
            "   return 1;\n"\
            "}",
            "unknown operator: BOOLEAN + BOOLEAN"
        },
        {
            "foobar;",
            "identifier not found: foobar"
        }
    };

    print_test_separator_line();
    environment_t *env;
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Test error handling for %s\n", test.input);
        env = create_env();
        monkey_object_t *evaluated = test_eval(test.input, env);
        test(evaluated->type == MONKEY_ERROR, "Expected MONKEY_ERROR to be returned, found %s\n",
            get_type_name(evaluated->type));
        monkey_error_t *err = (monkey_error_t *) evaluated;
        test(strcmp(err->message, test.message) == 0,
            "Expected error message %s, got %s\n", test.message, err->message);
        free_monkey_object(evaluated);
        env_free(env);
    }
}

static void
test_let_statements(void)
{
    environment_t *env;
    typedef struct {
        const char *input;
        long expected;
    } test_input;

    test_input tests[] = {
        {
            "let a = 5; a;", 5
        },
        {
            "let a = 5 * 5; a;", 25
        },
        {
            "let a = 5; let b = a; b;", 5
        },
        {
            "let a = 5; let b = a; let c = a + b + 5; c;", 15
        }
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        env = create_env();
        printf("Testing let statement for %s\n", test.input);
        monkey_object_t *evaluated = test_eval(test.input, env);
        test_integer_object(evaluated, test.expected);
        env_free(env);
    }
}

static void
test_function_object(void)
{
    const char *input = "fn(x) { x + 2;};";
    environment_t *env = create_env();
    print_test_separator_line();
    printf("Testing function object\n");
    monkey_object_t *evaluated = test_eval(input, env);
    test(evaluated->type == MONKEY_FUNCTION,
        "Expected object of type MONKEY_FUNCTION, found %s\n",
        get_type_name(evaluated->type));
    monkey_function_t *function_obj = (monkey_function_t *) evaluated;
    test(function_obj->parameters->length == 1,
        "Expected 1 parameters in the function, found %zu\n", function_obj->parameters->length);
    identifier_t *first_param = (identifier_t *) function_obj->parameters->head->data;
    test(strcmp(first_param->value, "x") == 0, "Expected param name to be x, found %s\n", first_param->value);
    const char *expected_body = "(x + 2)";
    char *actual_body = function_obj->body->statement.node.string(function_obj->body);
    test(strcmp(expected_body, actual_body) == 0, "Expected function body %s, found %s\n",
        expected_body, actual_body);
    free(actual_body);
    env_free(env);
    free_monkey_object(evaluated);
}

static void
test_function_application(void)
{
    typedef struct {
        const char *input;
        long expected;
    } test_input;

    test_input tests[] = {
        {
            "let identity = fn(x) {x;}; identity(5);",
            5
        },
        {
            "let identity = fn(x) { return x;}; identity(5);",
            5
        },
        {
            "let double = fn(x) { x * 2;} double(5);",
            10
        },
        {
            "let add = fn(x, y) {x + y;}; add(5, 5);",
            10
        },
        {
            "let add = fn(x, y) {x + y;}; add(5 + 5, add(5, 5));", 20
        },
        {
            "fn(x) { x; }(5)",
            5
        }
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing function application for %s\n", test.input);
        environment_t *env = create_env();
        monkey_object_t *evaluated = test_eval(test.input, env);
        test_integer_object(evaluated, test.expected);
        env_free(env);
    }
}

int
main(int argc, char **argv)
{
    // test_eval_integer_expression();
    // test_eval_bool_expression();
    // test_bang_operator();
    // test_if_else_expressions();
    // test_return_statements();
    // test_error_handling();
    // test_let_statements();
    // test_function_object();
    test_function_application();
    return 0;
}
