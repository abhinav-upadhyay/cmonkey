#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "cmonkey_utils.h"

static char *
inspect(monkey_object_t *obj)
{
    monkey_int_t *int_obj;
    monkey_bool_t *bool_obj;
    monkey_null_t *null_obj;

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
    monkey_bool_t *bool_obj;
    bool_obj = malloc(sizeof(*bool_obj));
    if (bool_obj == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    bool_obj->object.inspect = inspect;
    bool_obj->object.type = MONKEY_BOOL;
    bool_obj->value = value;
    return bool_obj;
}

monkey_null_t *
create_monkey_null(void)
{
    monkey_null_t *null_obj;
    null_obj = malloc(sizeof(*null_obj));
    if (null_obj == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    null_obj->object.inspect = inspect;
    null_obj->object.type = MONKEY_NULL;
    return null_obj;
}