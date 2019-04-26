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

#include <string.h>

#include "builtins.h"
#include "cmonkey_utils.h"

static monkey_object_t *len(cm_list *);
static monkey_object_t *first(cm_list *);
static monkey_object_t *last(cm_list *);
static monkey_object_t *rest(cm_list *);
static monkey_object_t *push(cm_list *);
static monkey_object_t *monkey_puts(cm_list *); //puts is a C function
static monkey_object_t *type(cm_list *);

static char *
builtin_inspect(monkey_object_t *object)
{
    return "builtin function";
}

monkey_builtin_t BUILTIN_LEN = {{MONKEY_BUILTIN, builtin_inspect}, len};
monkey_builtin_t BUILTIN_FIRST = {{MONKEY_BUILTIN, builtin_inspect}, first};
monkey_builtin_t BUILTIN_LAST = {{MONKEY_BUILTIN, builtin_inspect}, last};
monkey_builtin_t BUILTIN_REST = {{MONKEY_BUILTIN, builtin_inspect}, rest};
monkey_builtin_t BUILTIN_PUSH = {{MONKEY_BUILTIN, builtin_inspect}, push};
monkey_builtin_t BUILTIN_PUTS = {{MONKEY_BUILTIN, builtin_inspect}, monkey_puts};
monkey_builtin_t BUILTIN_TYPE = {{MONKEY_BUILTIN, builtin_inspect}, type};

static monkey_object_t *
monkey_puts(cm_list *arguments)
{
    monkey_object_t *arg;
    char *s;
    cm_list_node *node = arguments->head;
    while (node != NULL) {
        arg = (monkey_object_t *) node->data;
        node = node->next;
        s = arg->inspect(arg);
        printf("%s\n", s);
        free(s);
    }
    return (monkey_object_t *) create_monkey_null();
}

static monkey_object_t *
type(cm_list *arguments)
{
    monkey_object_t *arg;
    if (arguments->length != 1) {
        return (monkey_object_t *)
            create_monkey_error("wrong number of arguments. got=%zu, want=1",
            arguments->length);
    }

    arg = (monkey_object_t *) arguments->head->data;
    const char *typename = get_type_name(arg->type);
    return (monkey_object_t *) create_monkey_string(typename, strlen(typename));
}

static monkey_object_t *
len(cm_list *arguments)
{
    monkey_string_t *str;
    monkey_array_t *array;
    monkey_hash_t *hash_obj;
    if (arguments->length != 1) {
        return (monkey_object_t *) create_monkey_error(
            "wrong number of arguments. got=%zu, want=1", arguments->length);
    }

    monkey_object_t *arg = (monkey_object_t *) arguments->head->data;
    switch (arg->type) {
        case MONKEY_STRING:
            str = (monkey_string_t *) arg;
            return (monkey_object_t *) create_monkey_int(str->length);
        case MONKEY_ARRAY:
            array = (monkey_array_t *) arg;
            return (monkey_object_t *) create_monkey_int(array->elements->length);
        case MONKEY_HASH:
            hash_obj = (monkey_hash_t *) arg;
            return (monkey_object_t *) create_monkey_int(hash_obj->pairs->nkeys);
        default:
            return (monkey_object_t *) create_monkey_error(
                "argument to len not supported, got %s", get_type_name(arg->type));
    }
}

static monkey_object_t *
first(cm_list *arguments)
{
    monkey_array_t *array;
    if (arguments->length != 1) {
        return (monkey_object_t *)
            create_monkey_error("wrong number of arguments. got=%zu, want=1",
            arguments->length);
    }

    monkey_object_t *arg = (monkey_object_t *) arguments->head->data;
    if (arg->type != MONKEY_ARRAY) {
        return (monkey_object_t *) create_monkey_error(
            "argument to first must be ARRAY, got %s", get_type_name(arg->type));
    }
    array = (monkey_array_t *) arg;
    if (array->elements->length > 0)
        return copy_monkey_object(array->elements->array[0]);
    else
        return (monkey_object_t *) create_monkey_null();
}

static monkey_object_t *
last(cm_list *arguments)
{
    monkey_array_t *array;
    if (arguments->length != 1) {
        return (monkey_object_t *)
            create_monkey_error("wrong number of arguments. got=%zu, want=1",
            arguments->length);
    }

    monkey_object_t *arg = (monkey_object_t *) arguments->head->data;
    if (arg->type != MONKEY_ARRAY) {
        return (monkey_object_t *) create_monkey_error(
            "argument to last must be ARRAY, got %s", get_type_name(arg->type)
        );
    }

    array = (monkey_array_t *) arg;
    if (array->elements->length > 0)
        return (monkey_object_t *) copy_monkey_object(cm_array_list_last(array->elements));
    else
        return (monkey_object_t *) create_monkey_null();

}

static monkey_object_t *
rest(cm_list *arguments)
{
    monkey_array_t *array;
    cm_array_list *rest_elements;

    if (arguments->length != 1) {
        return (monkey_object_t *)
            create_monkey_error("wrong number of arguments. got=%zu, want=1",
            arguments->length);
    }

    monkey_object_t *arg = (monkey_object_t *) arguments->head->data;
    if (arg->type != MONKEY_ARRAY) {
        return (monkey_object_t *) create_monkey_error("argument to rest must be ARRAY, got %s",
        get_type_name(arg->type));
    }

    array = (monkey_array_t *) arg;

    if (array->elements->length == 0) {
        return (monkey_object_t *) create_monkey_null();
    }

    rest_elements = cm_array_list_init(array->elements->length, &free_monkey_object);
    for (size_t i = 1; i < array->elements->length; i++) {
        monkey_object_t *obj = copy_monkey_object(array->elements->array[i]);
        cm_array_list_add(rest_elements, obj);
    }
    monkey_array_t *rest_array = create_monkey_array(rest_elements);
    return (monkey_object_t *) rest_array;
}

static monkey_object_t *
push(cm_list *arguments)
{
    monkey_array_t *array;
    monkey_array_t *new_array;
    cm_array_list *new_list_elements;
    monkey_object_t *obj;

    if (arguments->length != 2) {
        return (monkey_object_t *)
            create_monkey_error("wrong number of arguments. got=%zu, want=2",
            arguments->length);
    }

    monkey_object_t *arg = (monkey_object_t *) arguments->head->data;
    if (arg->type != MONKEY_ARRAY) {
        return (monkey_object_t *)
            create_monkey_error("argument to push must be ARRAY, got %s",
            get_type_name(arg->type));
    }

    array = (monkey_array_t *) arg;
    new_list_elements = cm_array_list_init(arguments->length + 1, free_monkey_object);
    for (size_t i = 0; i < array->elements->length; i++) {
        obj = copy_monkey_object(array->elements->array[i]);
        cm_array_list_add(new_list_elements, obj);
    }

    obj = (monkey_object_t *) arguments->head->next->data;
    cm_array_list_add(new_list_elements, copy_monkey_object(obj));
    new_array = create_monkey_array(new_list_elements);
    return (monkey_object_t *) new_array;
}

monkey_builtin_t *
get_builtins(const char *name)
{
    if (strcmp(name, "len") == 0)
        return &BUILTIN_LEN;
    else if (strcmp(name, "first") == 0)
        return &BUILTIN_FIRST;
    else if (strcmp(name, "last") == 0)
        return &BUILTIN_LAST;
    else if (strcmp(name, "rest") == 0)
        return &BUILTIN_REST;
    else if (strcmp(name, "push") == 0)
        return &BUILTIN_PUSH;
    else if (strcmp(name, "puts") == 0)
        return &BUILTIN_PUTS;
    else if (strcmp(name, "type") == 0)
        return &BUILTIN_TYPE;
    else
        return NULL;
}
