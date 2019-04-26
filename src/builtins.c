#include <string.h>

#include "builtins.h"
#include "cmonkey_utils.h"

static monkey_object_t *len(cm_list *);
static monkey_object_t *first(cm_list *);
static monkey_object_t *last(cm_list *);
static monkey_object_t *rest(cm_list *);
static monkey_object_t *push(cm_list *);

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
    else
        return NULL;
}
