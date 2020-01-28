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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmonkey_utils.h"
#include "parser.h"
#include "object.h"
#include "opcode.h"

const monkey_bool_t MONKEY_TRUE_OBJ = {{MONKEY_BOOL, inspect, monkey_object_hash, monkey_object_equals}, true};
const monkey_bool_t MONKEY_FALSE_OBJ = {{MONKEY_BOOL, inspect, monkey_object_hash, monkey_object_equals}, false};
const monkey_null_t MONKEY_NULL_OBJ = {{MONKEY_NULL, inspect, NULL, NULL}};

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
    for (size_t i = 0; i < table->used_slots->length; i++) {
        size_t *index = (size_t *) table->used_slots->array[i];
        cm_list *entry_list = table->table[*index];
        cm_list_node *entry_node = entry_list->head;
        while (entry_node != NULL) {
            cm_hash_entry *entry = (cm_hash_entry *) entry_node->data;
            entry_node = entry_node->next;
            key_obj = (monkey_object_t *) entry->key;
            value_obj = (monkey_object_t *) entry->value;
            key_string = key_obj->inspect(key_obj);
            value_string = value_obj->inspect(value_obj);
            if (string == NULL)
                ret = asprintf(&temp, "%s: %s", key_string, value_string);
            else {
                ret = asprintf(&temp, "%s, %s: %s", string, key_string, value_string);
                free(string);
            }
            free(key_string);
            free(value_string);
            if (ret == -1)
                errx(EXIT_FAILURE, "malloc failed");
            string = temp;
            temp = NULL;
        }
    }
    ret = asprintf(&temp, "{%s}", string);
    free(string);
    if (ret == -1)
        errx(EXIT_FAILURE, "malloc failed");
    return temp;
}

char *
inspect(monkey_object_t *obj)
{
    monkey_int_t *int_obj;
    monkey_bool_t *bool_obj;
    monkey_return_value_t *ret_obj;
    monkey_error_t *err_obj;
    monkey_array_t *array;
    monkey_hash_t *hash_obj;
    monkey_compiled_fn_t *compiled_fn;
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
            return strdup(((monkey_string_t *) obj)->value);
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
        case MONKEY_COMPILED_FUNCTION:
            compiled_fn = (monkey_compiled_fn_t *) obj;
            ret = asprintf(&string, "compiled function %p", compiled_fn);
            if (ret == -1)
                err(EXIT_FAILURE, "malloc failed");
            return string;
    }
}

static _Bool
array_equals(monkey_array_t *arr1, monkey_array_t *arr2)
{
    if (arr1->elements->length != arr2->elements->length)
        return false;
    for (size_t i = 0; i < arr1->elements->length; i++) {
        if (!monkey_object_equals(arr1->elements->array[i], arr2->elements->array[i]))
            return false;
    }
    return true;
}

static _Bool
hash_equals(monkey_hash_t *hash1, monkey_hash_t *hash2)
{
    if (hash1->pairs->nkeys != hash2->pairs->nkeys)
        return false;
    for (size_t i = 0; i < hash1->pairs->nkeys; i++) {
        size_t *index1 = (size_t *) hash1->pairs->used_slots->array[i];
        size_t *index2 = (size_t *) hash2->pairs->used_slots->array[i];
        if (*index1 != *index2) {
            /** since we are using same hash functions, and both tables
             *  are supposed to have equal length, they must have same
             *  objects at same indices
             */
            return false;
        }
    }
    return true;
}

static _Bool
instructions_equals(instructions_t *ins1, instructions_t *ins2)
{
    if (ins1->length != ins2->length)
        return false;
    for (size_t i = 0; i < ins1->length; i++) {
        if (ins1->bytes[i] != ins2->bytes[i])
            return false;
    }
    return true;
}

_Bool
monkey_object_equals(void *o1, void *o2)
{
    monkey_object_t *obj1 = (monkey_object_t *) o1;
    monkey_object_t *obj2 = (monkey_object_t *) o2;
    if (obj1->type != obj2->type)
        return false;

    monkey_array_t *array1;
    monkey_array_t *array2;
    monkey_builtin_t *builtin1;
    monkey_builtin_t *builtin2;
    monkey_error_t *err1;
    monkey_error_t *err2;
    monkey_function_t *function1;
    monkey_function_t *function2;
    monkey_hash_t *hash1;
    monkey_hash_t *hash2;
    monkey_int_t *int1;
    monkey_int_t *int2;
    monkey_string_t *str1;
    monkey_string_t *str2;
    monkey_return_value_t *ret1;
    monkey_return_value_t *ret2;
    monkey_compiled_fn_t *fn1;
    monkey_compiled_fn_t *fn2;
    switch (obj1->type) {
        case MONKEY_ARRAY:
            array1 = (monkey_array_t *) obj1;
            array2 = (monkey_array_t *) obj2;
            return array_equals(array1, array2);
        case MONKEY_BOOL:
            return ((monkey_bool_t *) obj1)->value == ((monkey_bool_t *) obj2)->value;
        case MONKEY_BUILTIN:
            builtin1 = (monkey_builtin_t *) obj1;
            builtin2 = (monkey_builtin_t *) obj2;
            return builtin1 == builtin2; // these are static pointers
        case MONKEY_ERROR:
            err1 = (monkey_error_t *) obj1;
            err2 = (monkey_error_t *) obj2;
            return strcmp(err1->message, err2->message) == 0;
        case MONKEY_FUNCTION:
            function1 = (monkey_function_t *) obj1;
            function2 = (monkey_function_t *) obj2;
            return function1 == function2; //should we bother about this?
        case MONKEY_HASH:
            hash1 = (monkey_hash_t *) obj1;
            hash2 = (monkey_hash_t *) obj2;
            return hash_equals(hash1, hash2);
        case MONKEY_INT:
            int1 = (monkey_int_t *) obj1;
            int2 = (monkey_int_t *) obj2;
            return int1->value == int2->value;
        case MONKEY_STRING:
            str1 = (monkey_string_t *) obj1;
            str2 = (monkey_string_t *) obj2;
            return strcmp(str1->value, str2->value) == 0;
        case MONKEY_RETURN_VALUE:
            ret1 = (monkey_return_value_t *) obj1;
            ret2 = (monkey_return_value_t *) obj2;
            return monkey_object_equals(ret1->value, ret2->value);
        case MONKEY_NULL:
            return obj1 == obj2;
        case MONKEY_COMPILED_FUNCTION:
            fn1 = (monkey_compiled_fn_t *) obj1;
            fn2 = (monkey_compiled_fn_t *) obj2;
            return instructions_equals(fn1->instructions, fn2->instructions);
    }
}

size_t
monkey_object_hash(void *object)
{
    monkey_object_t *monkey_object = (monkey_object_t *) object;
    monkey_string_t *str_obj;
    monkey_int_t *int_obj;
    monkey_bool_t *bool_obj;
    switch (monkey_object->type) {
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
        err(EXIT_FAILURE, "malloc failed");
    int_obj->object.inspect = inspect;
    int_obj->object.type = MONKEY_INT;
    int_obj->object.hash = monkey_object_hash;
    int_obj->object.equals = monkey_object_equals;
    int_obj->value = value;
    return int_obj;
}

monkey_compiled_fn_t *
create_monkey_compiled_fn(instructions_t *ins, size_t num_locals, size_t num_args)
{
    monkey_compiled_fn_t *compiled_fn;
    compiled_fn = malloc(sizeof(*compiled_fn));
    if (compiled_fn == NULL)
        err(EXIT_FAILURE, "malloc failed");
    compiled_fn->instructions = ins;
    compiled_fn->num_locals = num_locals;
    compiled_fn->num_args = num_args;
    compiled_fn->object.type = MONKEY_COMPILED_FUNCTION;
    compiled_fn->object.inspect = inspect;
    compiled_fn->object.equals = monkey_object_equals;
    compiled_fn->object.hash = NULL;
    return compiled_fn;
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
    ret->object.equals = monkey_object_equals;
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
    error->object.equals = monkey_object_equals;
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
    monkey_hash_t *hash_obj;
    monkey_compiled_fn_t *compiled_fn;
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
        case MONKEY_HASH:
            hash_obj = (monkey_hash_t *) object;
            cm_hash_table_free(hash_obj->pairs);
            free(hash_obj);
            break;
        case MONKEY_COMPILED_FUNCTION:
            compiled_fn = (monkey_compiled_fn_t *) object;
            instructions_free(compiled_fn->instructions);
            free(compiled_fn);
            break;
        case MONKEY_BUILTIN:
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
    monkey_hash_t *hash_obj;
    monkey_compiled_fn_t *compiled_fn;
    if (object == NULL)
        return (monkey_object_t *) create_monkey_null();

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
        case MONKEY_HASH:
            hash_obj = (monkey_hash_t *) object;
            return (monkey_object_t *) create_monkey_hash(cm_hash_table_copy(hash_obj->pairs,
                _copy_monkey_object, _copy_monkey_object));
        case MONKEY_COMPILED_FUNCTION:
            compiled_fn = (monkey_compiled_fn_t *) object;
            return (monkey_object_t *) create_monkey_compiled_fn(copy_instructions(compiled_fn->instructions),
                compiled_fn->num_locals, compiled_fn->num_args);
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
    function->object.equals = monkey_object_equals;
    return function;
}

monkey_string_t *
create_monkey_string(const char *value, size_t length)
{
    monkey_string_t *string_obj;
    string_obj = malloc(sizeof(*string_obj));
    if (string_obj == NULL)
        err(EXIT_FAILURE, "malloc failed");
    if (value != NULL) {
        string_obj->value = malloc(sizeof(*value) * (length + 1));
        if (value == NULL)
            err(EXIT_FAILURE, "malloc failed");
        memcpy(string_obj->value, value, length);
        string_obj->value[length] = 0;
        if (string_obj->value == NULL)
            err(EXIT_FAILURE, "malloc failed");
        string_obj->length = length;
    } else {
        string_obj->value = NULL;
        string_obj->length = 0;
    }
    string_obj->object.type = MONKEY_STRING;
    string_obj->object.hash = monkey_object_hash;
    string_obj->object.inspect = inspect;
    string_obj->object.equals = monkey_object_equals;
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
    array->object.equals = monkey_object_equals;
    return array;
}

monkey_hash_t *
create_monkey_hash(cm_hash_table *pairs)
{
    monkey_hash_t *hash_obj = malloc(sizeof(*hash_obj));
    if (hash_obj == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    hash_obj->object.type = MONKEY_HASH;
    hash_obj->object.inspect = inspect;
    hash_obj->object.hash = NULL;
    hash_obj->object.equals = monkey_object_equals;
    hash_obj->pairs = pairs;
    return hash_obj;
}