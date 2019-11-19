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
}

void
test_string_object(monkey_object_t *obj, char *expected_value, size_t expected_length)
{
    monkey_string_t *str_obj = (monkey_string_t *) obj;
    test(str_obj->length == expected_length,
        "Expected string length %zu, got %zu\n",
        expected_length, str_obj->length);
    test(strncmp(str_obj->value, expected_value, expected_length) == 0,
        "Expected string %s, got %s\n", expected_value, str_obj->value);
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
    else if (expected->type == MONKEY_NULL)
        test_null_object(expected);
    else if (expected->type == MONKEY_STRING) {
        monkey_string_t *expected_str_obj = (monkey_string_t *) expected;
        test_string_object(obj, expected_str_obj->value, expected_str_obj->length);
    }
}
