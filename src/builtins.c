#include <string.h>

#include "builtins.h"
#include "cmonkey_utils.h"


static monkey_object_t *
len(cm_list *arguments)
{
    monkey_string_t *str;
    monkey_array_t *array;
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
        default:
            return (monkey_object_t *) create_monkey_error(
                "argument to len not supported, got %s", get_type_name(arg->type));
    }
}

static char *
builtin_inspect(monkey_object_t *object)
{
    return "builtin function";
}

monkey_builtin_t LEN_BUILTIN = {{MONKEY_BUILTIN, builtin_inspect}, len};

monkey_builtin_t *
get_builtins(const char *name)
{
    if (strcmp(name, "len") == 0)
        return &LEN_BUILTIN;
    else
        return NULL;
}