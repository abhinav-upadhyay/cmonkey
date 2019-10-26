#include <stdbool.h>
#include "object.h"
#include "test_utils.h"

void
test_boolean_object(monkey_object_t *object, _Bool expected_value)
{
    test(object->type == MONKEY_BOOL, "Expected object of type %s, got %s\n",
        get_type_name(MONKEY_BOOL), get_type_name(object->type));
    monkey_bool_t *bool_obj = (monkey_bool_t *) object;
    test(bool_obj->value == expected_value, "Expected bool value %s, got %s\n",
        bool_to_string(expected_value), bool_to_string(bool_obj->value));
}

void
test_integer_object(monkey_object_t *object, long expected_value)
{
    test(object->type == MONKEY_INT, "Expected object of type %s, got %s\n",
        get_type_name(MONKEY_INT), get_type_name(object->type));
    
    monkey_int_t *int_obj = (monkey_int_t *) object;
    test(int_obj->value == expected_value,
        "Expected integer object value to be %ld, found %ld\n",
        expected_value, int_obj->value);
}

void
test_null_object(monkey_object_t *object)
{
    test(object->type == MONKEY_NULL, "Expected a MONKEY_NULL object, found %s\n",
        get_type_name(object->type));
    test(object == (monkey_object_t *) create_monkey_null(), "object is not NULL\n");
}

void
test_monkey_object(monkey_object_t *obj, monkey_object_t *expected)
{
    test(obj->type == expected->type, "Expected object of type %s, got %s\n",
        get_type_name(expected->type), get_type_name(obj->type));
    if (expected->type == MONKEY_INT)
        test_integer_object(obj, ((monkey_int_t *) expected)->value);
    else if (expected->type == MONKEY_BOOL)
        test_boolean_object(obj, ((monkey_bool_t *) expected)->value);
}
