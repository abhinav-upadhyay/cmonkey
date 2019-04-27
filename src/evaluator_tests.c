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
        {"(1 > 2) == false", true},
        {"5 > 10 || 5 > 1", true},
        {"2 + 1 > 1 && 3 - 1 != 3", true},
        {"2 != 1 && false", false},
        {"2 != 1 || false", true}
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
test_while_expressions(void)
{
    environment_t *env;
    typedef struct {
        const char *input;
        monkey_object_t *expected;
    } test_input;

    test_input tests[] = {
        {
            "let x = 10;\n"\
            "while (x > 1) {\n"\
            "   let x = x -1;\n"\
            "   x;\n"\
            "};",
            (monkey_object_t *) create_monkey_int(1)
        },
        {
            "let x = 10;\n"\
            "while (x > 10) {\n"\
            "   let x = x -1;\n"\
            "   x;\n"\
            "};",
            (monkey_object_t *) create_monkey_null()
        }
    };
    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing while expression evaluation for %s\n", test.input);
        env = create_env();
        monkey_object_t *evaluated = test_eval(test.input, env);
        test(evaluated->type == test.expected->type,
            "Expected %s, got %s\n",
            get_type_name(test.expected->type), get_type_name(evaluated->type));
        if (evaluated->type == MONKEY_INT) {
            monkey_int_t *actual = (monkey_int_t *) evaluated;
            monkey_int_t *expected_int = (monkey_int_t *) test.expected;
            test(actual->value == expected_int->value, "Expected value %ld got %ld\n",
                expected_int->value, actual->value);
        }
        env_free(env);
        free_monkey_object(evaluated);
        free_monkey_object(test.expected);
    }
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
        },
        {
            "\"Hello\" - \"World\"",
            "unknown operator: STRING - STRING"
        },
        {
            "{\"name\": \"Monkey\"}[fn(x) {x}]",
            "unusable as a hash key: FUNCTION"
        },
        {
            "let x = 10;\n"\
            "let y = if (x == 10) {\n"\
            "   let x = x + 1;\n"\
            "} else {\n"\
            "   let x = x * 2;\n"\
            "};\n"\
            "y + 1",
            "type mismatch: NULL + INTEGER"
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

static void
test_string_literal(void)
{
    const char *input = "\"Hello, world!\"";
    environment_t *env = create_env();
    monkey_object_t *evaluated = test_eval(input, env);
    print_test_separator_line();
    printf("Testing string literal evaluation\n");
    test(evaluated->type == MONKEY_STRING,
        "Expected object of type MONKEY_STRING, got %s\n",
        get_type_name(evaluated->type));
    monkey_string_t *str = (monkey_string_t *) evaluated;
    test(strcmp(str->value, "Hello, world!") == 0,
        "Expected string literal value \"Hello, world!\", found \"%s\"\n",
        str->value);
    free_monkey_object(str);
    env_free(env);
}

static void
test_string_concatenation(void)
{
    const char *input = "\"Hello,\" + \" \" + \"world!\"";
    environment_t * env = create_env();
    monkey_object_t *evaluated = test_eval(input, env);
    print_test_separator_line();
    printf("Testing string concatenation evaluation\n");
    test(evaluated->type == MONKEY_STRING,
        "Expected object of type MONKEY_STRING, got %s\n",
        get_type_name(evaluated->type));
    monkey_string_t *str = (monkey_string_t *) evaluated;
    test(strcmp(str->value, "Hello, world!") == 0,
        "Expected string literal value \"Hello, world!\", found \"%s\"\n",
        str->value);
    free_monkey_object(str);
    env_free(env);
}

static void
test_int_array(monkey_array_t *actual, monkey_array_t *expected)
{
    test(expected->elements->length == actual->elements->length,
        "Expected length of array %zu, got %zu\n", expected->elements->length,
        actual->elements->length);
    for (size_t i = 0; i < expected->elements->length; i++) {
        monkey_object_t *obj = (monkey_object_t *) expected->elements->array[i];
        test(obj->type == MONKEY_INT,
            "Expected element at %zu index to be INTEGER, got %s\n", i,
            get_type_name(obj->type));
        monkey_int_t *act_int = (monkey_int_t *) obj;
        monkey_int_t *exp_int = (monkey_int_t *) expected->elements->array[i];
        test(act_int->value == exp_int->value,
            "Expected value %ld at index %zu, got %ld\n", exp_int->value, i, act_int->value);
    }
    free_monkey_object(actual);
}

static monkey_array_t *
create_int_array(int *int_arr, size_t length)
{
    cm_array_list *array_list = cm_array_list_init(length, free_monkey_object);
    for (size_t i = 0; i < length; i++) {
        cm_array_list_add(array_list, (void *) create_monkey_int(int_arr[i]));
    }
    return create_monkey_array(array_list);
}

static void
test_builtins(void)
{
    typedef struct {
        const char *input;
        monkey_object_t *expected;
    } test_input;

    test_input tests[] = {
        {"len(\"\")", (monkey_object_t *) create_monkey_int(0)},
        {"len(\"four\")", (monkey_object_t *) create_monkey_int(4)},
        {"len(\"hello world\")", (monkey_object_t *) create_monkey_int(11)},
        {"len(1)", (monkey_object_t *) create_monkey_error("argument to len not supported, got INTEGER")},
        {"len(\"one\", \"two\")", (monkey_object_t *) create_monkey_error("wrong number of arguments. got=2, want=1")},
        {"len([1, 2, 3])", (monkey_object_t *) create_monkey_int(3)},
        {"len([])", (monkey_object_t *) create_monkey_int(0)},
        {"first([1, 2, 3])", (monkey_object_t *) create_monkey_int(1)},
        {"first([])", (monkey_object_t *) create_monkey_null()},
        {"first(1)", (monkey_object_t *) create_monkey_error("argument to first must be ARRAY, got INTEGER")},
        {"last([1, 2, 3])", (monkey_object_t *) create_monkey_int(3)},
        {"last([])", (monkey_object_t *) create_monkey_null()},
        {"last(1)", (monkey_object_t *) create_monkey_error("argument to last must be ARRAY, got INTEGER")},
        {"rest([1, 2, 3])", (monkey_object_t *) create_int_array((int[]) {2, 3}, 2)},
        {"rest([])", (monkey_object_t *) create_monkey_null()},
        {"push([], 1)", (monkey_object_t *) create_int_array((int[]){1}, 1)},
        {"push(1, 1)", (monkey_object_t *) create_monkey_error("argument to push must be ARRAY, got INTEGER")},
        {"type(10)", (monkey_object_t *) create_monkey_string("INTEGER", 7)},
        {"type(10, 1)", (monkey_object_t *) create_monkey_error("wrong number of arguments. got=2, want=1")}
    };

    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    print_test_separator_line();
    monkey_int_t *actual_int;
    monkey_array_t *actual_array;
    monkey_array_t *expected_array;
    monkey_error_t *actual_err;
    monkey_error_t *expected_err;
    monkey_string_t *expected_str;
    monkey_string_t *actual_str;
    monkey_object_t *obj;
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing builtin function %s\n", test.input);
        environment_t *env = create_env();
        monkey_object_t *evaluated = test_eval(test.input, env);
        switch (test.expected->type) {
            case MONKEY_INT:
                actual_int = (monkey_int_t *) evaluated;
                test_integer_object(evaluated, actual_int->value);
                free_monkey_object(test.expected);
                break;
            case MONKEY_ERROR:
                actual_err = (monkey_error_t *) evaluated;
                expected_err = (monkey_error_t *) test.expected;
                test(strcmp(actual_err->message, expected_err->message) == 0,
                    "Expected error message %s, got %s\n", expected_err->message,
                    actual_err->message);
                free_monkey_object(evaluated);
                free_monkey_object(test.expected);
                break;
            case MONKEY_ARRAY:
                actual_array = (monkey_array_t *) evaluated;
                expected_array = (monkey_array_t *) test.expected;
                test_int_array(actual_array, expected_array);
                free_monkey_object(test.expected);
                break;
            case MONKEY_NULL:
                obj = (monkey_object_t *) evaluated;
                test(obj->type == MONKEY_NULL,
                    "Expected null object, got %s\n", get_type_name(obj->type));
                break;
            case MONKEY_STRING:
                expected_str = (monkey_string_t *) test.expected;
                actual_str = (monkey_string_t *) evaluated;
                test(strcmp(expected_str->value, actual_str->value) == 0,
                    "Expected value %s, got %s\n", expected_str->value, actual_str->value);
                free_monkey_object(test.expected);
                free_monkey_object(evaluated);
                break;
            default:
                err(EXIT_FAILURE, "Unknown type for expected");
        }
        env_free(env);
    }
}

static void
test_array_literals(void)
{
    const char *input = "[1, 2 * 2, 3 + 3]";
    print_test_separator_line();
    printf("Testing array literal evaluation\n");
    environment_t *env = create_env();
    monkey_object_t *evaluated = test_eval(input, env);
    test(evaluated->type == MONKEY_ARRAY, "Expected MONKEY_ARRAY, got %s\n",
        get_type_name(evaluated->type));
    monkey_array_t *array = (monkey_array_t *) evaluated;
    test(array->elements->length == 3, "Expected 3 elements in array object, got %zu\n",
        array->elements->length);
    test_integer_object(copy_monkey_object(array->elements->array[0]), 1);
    test_integer_object(copy_monkey_object(array->elements->array[1]), 4);
    test_integer_object(copy_monkey_object(array->elements->array[2]), 6);
    free_monkey_object(evaluated);
    env_free(env);
}

static void
test_array_index_expressions(void)
{
    typedef struct {
        const char *input;
        monkey_object_t *expected;
    } test_input;

    test_input tests[] = {
        {"[1, 2, 3][0]", (monkey_object_t *) create_monkey_int(1)},
        {"[1, 2, 3][1]", (monkey_object_t *) create_monkey_int(2)},
        {"[1, 2, 3][2]", (monkey_object_t *) create_monkey_int(3)},
        {"let i = 0; [1][i]", (monkey_object_t *) create_monkey_int(1)},
        {"[1, 2, 3][1 + 1]", (monkey_object_t *) create_monkey_int(3)},
        {"let my_array = [1, 2, 3]; my_array[2];", (monkey_object_t *) create_monkey_int(3)},
        {"let my_array = [1, 2, 3]; my_array[0] + my_array[1] + my_array[2]",
            (monkey_object_t *) create_monkey_int(6)},
        {"let my_array = [1, 2, 3]; let i = my_array[0]; my_array[i]",
            (monkey_object_t *) create_monkey_int(2)},
        {"[1, 2, 3][3]", (monkey_object_t *) create_monkey_null()},
        {"[1, 2, 3][-1]", (monkey_object_t *) create_monkey_null()}
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing index expression evaluation for %s\n", test.input);
        environment_t *env = create_env();
        monkey_object_t *evaluated = test_eval(test.input, env);
        test(evaluated->type == test.expected->type, "Expected object %s, got %s\n",
            get_type_name(test.expected->type), get_type_name(evaluated->type));
        if (test.expected->type == MONKEY_INT) {
            test_integer_object(evaluated, ((monkey_int_t *) test.expected)->value);
            free_monkey_object(test.expected);
        } else {
            test_null_object(evaluated);
        }
        env_free(env);
    }
}

static void
test_enclosing_env(void)
{
    const char *input = "let first = 10;\n"\
        "let second = 10;\n"\
        "let third = 10;\n\n"\
        "let ourfunction = fn(first) {\n"\
        "   let second = 20;\n"\
        "   first + second + third;\n"\
        "};\n"\
        "ourfunction(20) + first + second;";
    print_test_separator_line();
    printf("Testing enclosed environment\n");
    environment_t *env = create_env();
    monkey_object_t *evaluated = test_eval(input, env);
    printf("Enclosed environment test passed\n");
    test_integer_object(evaluated, 70);
    env_free(env);
}

static void
test_hash_literals(void)
{
    const char *input = "let two = \"two\";\n"\
        "{\"one\": 10 - 9,\n"\
        "two: 1 + 1,\n"\
        "\"thr\" + \"ee\": 6 / 2,\n"\
        "4: 4,\n"\
        "true: 5,\n"\
        "false: 6\n"\
        "}";
    cm_hash_table *expected = cm_hash_table_init(monkey_object_hash,
        monkey_object_equals, free_monkey_object, free_monkey_object);
    cm_hash_table_put(expected, create_monkey_string("one", 3), create_monkey_int(1));
    cm_hash_table_put(expected, create_monkey_string("two", 3), create_monkey_int(2));
    cm_hash_table_put(expected, create_monkey_string("three", 3), create_monkey_int(3));
    cm_hash_table_put(expected, create_monkey_int(4), create_monkey_int(4));
    cm_hash_table_put(expected, create_monkey_bool(true), create_monkey_int(5));
    cm_hash_table_put(expected, create_monkey_bool(false), create_monkey_int(6));
    print_test_separator_line();
    printf("Testing hash literal evaluation for %s\n", input);
    environment_t *env = create_env();
    monkey_object_t *evaluated = test_eval(input, env);
    test(evaluated->type == MONKEY_HASH,
        "Expected a HASH object, got %s\n", get_type_name(evaluated->type));
    monkey_hash_t *hash_obj = (monkey_hash_t *) evaluated;
    test(hash_obj->pairs->nkeys == expected->nkeys,
        "Expected %zu entries in table, got %zu\n",
        expected->nkeys, hash_obj->pairs->nkeys);
    for (size_t i = 0; i < expected->used_slots->length; i++) {
        size_t *index = (size_t *) expected->used_slots->array[i];
        cm_list *entry_list = expected->table[*index];
        cm_list_node *entry_node = entry_list->head;
        while (entry_node != NULL) {
            cm_hash_entry *entry = (cm_hash_entry *) entry_node->data;
            monkey_object_t *expected_key = (monkey_object_t *) entry->key;
            monkey_object_t *expected_value = (monkey_object_t *) entry->value;
            monkey_object_t *actual_value = (monkey_object_t *) cm_hash_table_get(hash_obj->pairs, expected_key);
            char *key_string = expected_key->inspect(expected_key);
            test(actual_value != NULL,
                "Key %s not found in hash object\n", key_string);
            free(key_string);
            test_integer_object(copy_monkey_object(actual_value), ((monkey_int_t *) expected_value)->value);
            entry_node = entry_node->next;
        }
    }
    free_monkey_object(evaluated);
    cm_hash_table_free(expected);
    env_free(env);
}

static void
test_hash_index_expressions(void)
{
    typedef struct {
        const char *input;
        monkey_object_t *expected;
    } test_input;

    test_input tests[] = {
        {
            "{\"foo\": 5}[\"foo\"]",
            (monkey_object_t *) create_monkey_int(5)
        },
        {
            "{\"foo\": 5}[\"bar\"]",
            (monkey_object_t *) create_monkey_null()
        },
        {
            "let key = \"foo\"; {\"foo\": 5}[key]",
            (monkey_object_t *) create_monkey_int(5)
        },
        {
            "{}[\"foo\"]",
            (monkey_object_t *) create_monkey_null()
        },
        {
            "{5: 5}[5]",
            (monkey_object_t *) create_monkey_int(5)
        },
        {
            "{true: 5}[true]",
            (monkey_object_t *) create_monkey_int(5)
        },
        {
            "{false: 5}[false]",
            (monkey_object_t *) create_monkey_int(5)
        }
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    monkey_int_t *expected_int;
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing hash index expression for %s\n", test.input);
        environment_t *env = create_env();
        monkey_object_t *evaluated = test_eval(test.input, env);
        switch (test.expected->type) {
            case MONKEY_INT:
                expected_int = (monkey_int_t *) test.expected;
                test_integer_object(evaluated, expected_int->value);
                break;
            case MONKEY_NULL:
                test(evaluated->type == MONKEY_NULL, "Expected null value got %s\n",
                    get_type_name(evaluated->type));
                break;
            default:
                err(EXIT_FAILURE, "Unknown type: %s", get_type_name(test.expected->type));
        }
        free_monkey_object(test.expected);
        env_free(env);
    }
}

int
main(int argc, char **argv)
{
    test_eval_integer_expression();
    test_eval_bool_expression();
    test_bang_operator();
    test_if_else_expressions();
    test_return_statements();
    test_error_handling();
    test_let_statements();
    test_function_object();
    test_function_application();
    test_string_literal();
    test_string_concatenation();
    test_builtins();
    test_array_literals();
    test_array_index_expressions();
    test_enclosing_env();
    test_hash_literals();
    test_hash_index_expressions();
    test_while_expressions();
    return 0;
}
