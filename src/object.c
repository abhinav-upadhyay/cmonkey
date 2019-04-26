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
static size_t monkey_hash(monkey_object_t *);

monkey_bool_t MONKEY_TRUE_OBJ = {{MONKEY_BOOL, inspect, monkey_hash}, true};
monkey_bool_t MONKEY_FALSE_OBJ = {{MONKEY_BOOL, inspect, monkey_hash}, false};
monkey_null_t MONKEY_NULL_OBJ = {{MONKEY_NULL, inspect, NULL}};

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
join_expressions_list(cm_array_list *list)
{
    char *string = NULL;
    char *temp = NULL;
    char *elem_string;
    monkey_object_t *elem;
    int ret;
    for (size_t i = 0; i < list->length; i++) {
        elem = (monkey_object_t *) list->array[i];
        elem_string = elem->inspect(elem);
        if (string == NULL) {
            ret = asprintf(&temp, "%s", elem_string);
        } else {
            ret = asprintf(&temp, "%s, %s", string, elem_string);
            free(string);
        }
        free(elem_string);
        if (ret == -1)
            errx(EXIT_FAILURE, "malloc failed");
        string = temp;
        temp = NULL;
    }
    return string;
}

static char *
join_expressions_table(cm_hash_table *table)
{
    char *string = NULL;
    char *temp = NULL;
    char *key_string;
    char *value_string;
    monkey_object_t *key_obj;
    monkey_object_t *value_obj;
    int ret;
    for (size_t i = 0; i < table->nkeys; i++) {
        size_t *index = (size_t *) table->used_slots->array[i];
        cm_hash_entry *entry = (cm_hash_entry *) table->table[*index];
        key_obj = (monkey_object_t *) entry->key;
        value_obj = (monkey_object_t *) entry->value;
        key_string = key_obj->inspect(key_obj);
        value_string = value_obj->inspect(value_obj);
        if (string == NULL)
            ret = asprintf(&temp, "%s:%s", key_string, value_string);
        else {
            ret = asprintf(&temp, "%s, %s:%s", string, key_string, value_string);
            free(string);
        }
        free(key_string);
        free(value_string);
        if (ret == -1)
            errx(EXIT_FAILURE, "malloc failed");
        string = temp;
        temp = NULL;
    }
    ret = asprintf(&temp, "{%s}", string);
    free(string);
    if (ret == -1)
        errx(EXIT_FAILURE, "malloc failed");
    return temp;
}

static char *
inspect(monkey_object_t *obj)
{
    monkey_int_t *int_obj;
    monkey_bool_t *bool_obj;
    monkey_return_value_t *ret_obj;
    monkey_error_t *err_obj;
    monkey_array_t *array;
    monkey_hash_t *hash_obj;
    char *string = NULL;
    char *elements_string = NULL;
    int ret;

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
        case MONKEY_STRING:
            return ((monkey_string_t *) obj)->value;
        case MONKEY_BUILTIN:
            return strdup("builtin function");
        case MONKEY_ARRAY:
            array = (monkey_array_t *) obj;
            if (array->elements->length > 0)
                elements_string = join_expressions_list(array->elements);
            ret = asprintf(&string, "[%s]", elements_string? elements_string: "");
            if (elements_string != NULL)
                free(elements_string);
            if (ret == -1)
                errx(EXIT_FAILURE, "malloc failed");
            return string;
        case MONKEY_HASH:
            hash_obj = (monkey_hash_t *) obj;
            return join_expressions_table(hash_obj->pairs);
    }
}

static size_t
monkey_hash(monkey_object_t *object)
{
    monkey_string_t *str_obj;
    monkey_int_t *int_obj;
    monkey_bool_t *bool_obj;
    switch (object->type) {
        case MONKEY_STRING:
            str_obj = (monkey_string_t *) object;
            return string_hash_function(str_obj->value);
        case MONKEY_INT:
            int_obj = (monkey_int_t *) object;
            return int_hash_function(&int_obj->value);
        case MONKEY_BOOL:
            bool_obj = (monkey_bool_t *) object;
            return pointer_hash_function(bool_obj);
        default:
            // We don't expect to come here, since the hash field is set to
            // NULL for other object types
            return 0;
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
    int_obj->object.hash = monkey_hash;
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
    ret->object.hash = NULL;
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
    error->object.hash = NULL;
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
    monkey_string_t *str_obj;
    monkey_array_t *array;
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
        case MONKEY_STRING:
            str_obj = (monkey_string_t *) object;
            free(str_obj->value);
            free(str_obj);
            break;
        case MONKEY_ARRAY:
            array = (monkey_array_t *) object;
            cm_array_list_free(array->elements);
            free(array);
            break;
        default:
            free(object);
    }
}

static void *
_copy_monkey_object(void *v)
{
    return (void *) copy_monkey_object((monkey_object_t *) v);
}

monkey_object_t *
copy_monkey_object(monkey_object_t *object)
{
    monkey_int_t *int_obj;
    monkey_function_t *function_obj;
    monkey_string_t *str_obj;
    monkey_builtin_t *builtin;
    monkey_array_t *array_obj;
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
        case MONKEY_STRING:
            str_obj = (monkey_string_t *) object;
            return (monkey_object_t *) create_monkey_string(str_obj->value, str_obj->length);
        case MONKEY_BUILTIN:
            builtin = (monkey_builtin_t *) object;
            return (monkey_object_t *) create_monkey_builtin(builtin->function);
        case MONKEY_ARRAY:
            array_obj = (monkey_array_t *) object;
            return (monkey_object_t *) create_monkey_array(cm_array_list_copy(array_obj->elements, _copy_monkey_object));
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
    function->object.hash = NULL;
    return function;
}

monkey_string_t *
create_monkey_string(const char *value, size_t length)
{
    monkey_string_t *string_obj;
    string_obj = malloc(sizeof(*string_obj));
    if (string_obj == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    string_obj->value = strdup(value);
    if (string_obj->value == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    string_obj->length = length;
    string_obj->object.type = MONKEY_STRING;
    string_obj->object.hash = monkey_hash;
    string_obj->object.inspect = inspect;
    return string_obj;
}

monkey_builtin_t *
create_monkey_builtin(builtin_fn function)
{
    monkey_builtin_t *builtin = malloc(sizeof(*builtin));
    if (builtin == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    builtin->object.type = MONKEY_BUILTIN;
    builtin->object.inspect = inspect;
    builtin->object.hash = NULL;
    builtin->function = function;
    return builtin;
}

monkey_array_t *
create_monkey_array(cm_array_list *elements)
{
    monkey_array_t *array = malloc(sizeof(*array));
    if (array == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    array->object.type = MONKEY_ARRAY;
    array->object.inspect = inspect;
    array->object.hash = NULL;
    array->elements = elements;
    return array;
}