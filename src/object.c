#include <err.h>
#include <stdbool.h>
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

void
free_monkey_object(monkey_object_t *object)
{
    switch (object->type) {
        case MONKEY_BOOL:
        case MONKEY_NULL:
            break;
        case MONKEY_INT:
            free((monkey_int_t *) object);
            break;
        default:
            free(object);
    }

}