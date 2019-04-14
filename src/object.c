#include <err.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "cmonkey_utils.h"

static char *inspect(monkey_object_t *);

monkey_bool_t MONKEY_TRUE_OBJ = {{MONKEY_BOOL, inspect}, true};
monkey_bool_t MONKEY_FALSE_OBJ = {{MONKEY_BOOL, inspect}, false};
monkey_null_t MONKEY_NULL_OBJ = {{MONKEY_NULL, inspect}};

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

void
free_monkey_object(monkey_object_t *object)
{
    monkey_error_t *err_obj;
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
        default:
            free(object);
    }
}

monkey_object_t *
copy_monkey_object(monkey_object_t *object)
{
    monkey_int_t *int_obj;
    switch (object->type) {
        case MONKEY_BOOL:
        case MONKEY_NULL:
            return object;
        case MONKEY_INT:
            int_obj = (monkey_int_t *) object;
            return (monkey_object_t *) create_monkey_int(int_obj->value);
        default:
            return NULL;
    }
}