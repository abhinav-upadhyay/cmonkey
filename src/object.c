#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "cmonkey_utils.h"

monkey_bool_t *MONKEY_TRUE_OBJ = NULL;
monkey_bool_t *MONKEY_FALSE_OBJ = NULL;
monkey_null_t *MONKEY_NULL_OBJ = NULL;

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
    if (value) {
        if (MONKEY_TRUE_OBJ == NULL) {
            MONKEY_TRUE_OBJ = malloc(sizeof(*MONKEY_TRUE_OBJ));
            if (MONKEY_TRUE_OBJ == NULL)
                errx(EXIT_FAILURE, "malloc failed");
            MONKEY_TRUE_OBJ->object.inspect = inspect;
            MONKEY_TRUE_OBJ->object.type = MONKEY_BOOL;
            MONKEY_TRUE_OBJ->value = value;
        }
        return MONKEY_TRUE_OBJ;
    } else {
            if (MONKEY_FALSE_OBJ == NULL) {
            MONKEY_FALSE_OBJ = malloc(sizeof(*MONKEY_FALSE_OBJ));
            if (MONKEY_FALSE_OBJ == NULL)
                errx(EXIT_FAILURE, "malloc failed");
            MONKEY_FALSE_OBJ->object.inspect = inspect;
            MONKEY_FALSE_OBJ->object.type = MONKEY_BOOL;
            MONKEY_FALSE_OBJ->value = value;
            }
        return MONKEY_FALSE_OBJ;
    }
}

monkey_null_t *
create_monkey_null(void)
{
    if (MONKEY_NULL_OBJ == NULL) {
        monkey_null_t *null_obj;
        null_obj = malloc(sizeof(*null_obj));
        if (null_obj == NULL)
            errx(EXIT_FAILURE, "malloc failed");
        null_obj->object.inspect = inspect;
        null_obj->object.type = MONKEY_NULL;
        MONKEY_NULL_OBJ = null_obj;
    }
    return MONKEY_NULL_OBJ;
}

void
free_monkey_object(monkey_object_t *object)
{

}