#include <string.h>

#include "builtins.h"
#include "cmonkey_utils.h"


static monkey_object_t *
len(cm_list *arguments)
{
    if (arguments->length != 1) {
        return (monkey_object_t *) create_monkey_error(
            "wrong number of arguments. got=%zu, want=1", arguments->length);
    }

    monkey_object_t *arg = (monkey_object_t *) arguments->head->data;
    if (arg->type != MONKEY_STRING) {
        return (monkey_object_t *) create_monkey_error(
            "argument to len not supported, got %s", get_type_name(arg->type));
    }

    monkey_string_t *str = (monkey_string_t *) arg;
    return (monkey_object_t *) create_monkey_int(str->length);
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