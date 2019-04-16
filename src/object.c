#include <err.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmonkey_utils.h"
#include "parser.h"
#include "object.h"

static char *inspect(monkey_object_t *);

monkey_bool_t MONKEY_TRUE_OBJ = {{MONKEY_BOOL, inspect}, true};
monkey_bool_t MONKEY_FALSE_OBJ = {{MONKEY_BOOL, inspect}, false};
monkey_null_t MONKEY_NULL_OBJ = {{MONKEY_NULL, inspect}};

static char *
monkey_function_inspect(monkey_object_t *obj)
{
    monkey_function_t *function = (monkey_function_t *) obj;
    char *str = NULL;
    char *params_str = join_parameters_list(function->parameters);
    char *body_str = function->body->statement.node.string(function->body);
    int ret = asprintf(&str,"fn(%s) {\n%s\n}", params_str, body_str);
    free(params_str);
    free(body_str);
    if (ret == -1)
        errx(EXIT_FAILURE, "malloc failed");
    return str;
}

static char *
inspect(monkey_object_t *obj)
{
    monkey_int_t *int_obj;
    monkey_bool_t *bool_obj;
    monkey_null_t *null_obj;
    monkey_return_value_t *ret_obj;
    monkey_error_t *err_obj;

    switch (obj->type)
    {
        case MONKEY_INT:
            int_obj = (monkey_int_t *) obj;
            return long_to_string(int_obj->value);
        case MONKEY_BOOL:
            bool_obj = (monkey_bool_t *) obj;
            return bool_obj->value? strdup("true"): strdup("false");
        case MONKEY_NULL:
            return strdup("null");
        case MONKEY_RETURN_VALUE:
            ret_obj = (monkey_return_value_t *) obj;
            return ret_obj->value->inspect(ret_obj->value);
        case MONKEY_ERROR:
            err_obj = (monkey_error_t *) obj;
            return strdup(err_obj->message);
        case MONKEY_FUNCTION:
            return monkey_function_inspect(obj);
    }
}

monkey_int_t *
create_monkey_int(long value)
{
    monkey_int_t *int_obj;
    int_obj = malloc(sizeof(*int_obj));
    if (int_obj == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    int_obj->object.inspect = inspect;
    int_obj->object.type = MONKEY_INT;
    int_obj->value = value;
    return int_obj;
}

monkey_bool_t *
create_monkey_bool(_Bool value)
{
    if (value) {
        return &MONKEY_TRUE_OBJ;
    } else {
        return &MONKEY_FALSE_OBJ;
    }
}

monkey_null_t *
create_monkey_null(void)
{
    return &MONKEY_NULL_OBJ;
}

monkey_return_value_t *
create_monkey_return_value(monkey_object_t *value)
{
    monkey_return_value_t *ret;
    ret = malloc(sizeof(*ret));
    if (ret == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    ret->value = value;
    ret->object.type = MONKEY_RETURN_VALUE;
    ret->object.inspect = inspect;
    return ret;
}

monkey_error_t *
create_monkey_error(const char *fmt, ...)
{
    monkey_error_t *error;
    char *message = NULL;
    error = malloc(sizeof(*error));
    if (error == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    error->object.type = MONKEY_ERROR;
    error->object.inspect = inspect;
    va_list args;
    va_start(args, fmt);
    int ret = vasprintf(&message, fmt, args);
    if (ret == -1)
        errx(EXIT_FAILURE, "malloc failed");
    va_end(args);

    error->message = message;
    return error;
}

static void
free_monkey_function_object(monkey_function_t *function_obj)
{
    free_statement((statement_t *) function_obj->body);
    cm_list_free(function_obj->parameters, free_expression);
    free(function_obj);
}

void
free_monkey_object(void *v)
{
    monkey_object_t *object = (monkey_object_t *) v;
    monkey_error_t *err_obj;
    monkey_return_value_t *return_value;
    switch (object->type) {
        case MONKEY_BOOL:
        case MONKEY_NULL:
            break;
        case MONKEY_INT:
            free((monkey_int_t *) object);
            break;
        case MONKEY_ERROR:
            err_obj = (monkey_error_t *) object;
            free(err_obj->message);
            free(err_obj);
            break;
        case MONKEY_FUNCTION:
            free_monkey_function_object((monkey_function_t *) object);
            break;
        case MONKEY_RETURN_VALUE:
            return_value = (monkey_return_value_t *) object;
            free_monkey_object(return_value->value);
            free(return_value);
            break;
        default:
            free(object);
    }
}

monkey_object_t *
copy_monkey_object(monkey_object_t *object)
{
    monkey_int_t *int_obj;
    monkey_function_t *function_obj;
    switch (object->type) {
        case MONKEY_BOOL:
        case MONKEY_NULL:
            return object;
        case MONKEY_INT:
            int_obj = (monkey_int_t *) object;
            return (monkey_object_t *) create_monkey_int(int_obj->value);
        case MONKEY_FUNCTION:
            function_obj = (monkey_function_t *) object;
            return (monkey_object_t *) create_monkey_function(
                function_obj->parameters, function_obj->body, function_obj->env);
        default:
            return NULL;
    }
}


monkey_function_t *
create_monkey_function(cm_list *parameters, block_statement_t *body, environment_t *env)
{
    monkey_function_t *function;
    function = malloc(sizeof(*function));
    if (function == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    function->parameters = copy_parameters(parameters);
    function->body = (block_statement_t *) copy_statement((statement_t *) body);
    function->env = env;
    function->object.type = MONKEY_FUNCTION;
    function->object.inspect = inspect;
    return function;
}