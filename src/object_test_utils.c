#include <stdbool.h>
#include <string.h>

#include "object.h"
#include "object_test_utils.h"
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
    } else if (expected->type == MONKEY_ARRAY)
        test_array_object(obj, expected);
    else if (expected->type == MONKEY_HASH)
        test_hash_object(obj, expected);
}

void
test_array_object(monkey_object_t *actual, monkey_object_t *expected)
{
    monkey_array_t *actual_arr = (monkey_array_t *) actual;
    monkey_array_t *expected_arr = (monkey_array_t *) expected;
    test(actual_arr->elements->length == expected_arr->elements->length,
        "Expected array size %zu, got %zu\n",
        expected_arr->elements->length, actual_arr->elements->length);
    for (size_t i = 0; i < actual_arr->elements->length; i++) {
        monkey_object_t *actual_obj = (monkey_object_t *) cm_array_list_get(actual_arr->elements, i);
        monkey_object_t *expected_obj = (monkey_object_t *) cm_array_list_get(expected_arr->elements, i);
        test_monkey_object(actual_obj, expected_obj);
    }
}

void
test_hash_object(monkey_object_t *actual, monkey_object_t *expected)
{
    monkey_hash_t *actual_hash = (monkey_hash_t *) actual;
    monkey_hash_t *expected_hash = (monkey_hash_t *) expected;
    test(actual_hash->pairs->nkeys == expected_hash->pairs->nkeys,
        "Expected hash size %zu, got %zu\n", expected_hash->pairs->nkeys,
        actual_hash->pairs->nkeys);
    cm_array_list *expected_keys = cm_hash_table_get_keys(expected_hash->pairs);
    cm_array_list *actual_keys = cm_hash_table_get_keys(actual_hash->pairs);
    if (expected_keys == NULL) {
        test(actual_keys == NULL, "Expected 0 keys in hash, got %zu\n", actual_keys->length);
    } else {
        for (size_t i = 0; i < expected_keys->length; i++) {
            monkey_object_t *key = cm_array_list_get(expected_keys, i);
            monkey_object_t *expected_value = (monkey_object_t *) cm_hash_table_get(expected_hash->pairs, key);
            monkey_object_t *actual_value = (monkey_object_t *) cm_hash_table_get(actual_hash->pairs, key);
            char *key_string = key->inspect(key);
            test(actual_value != NULL, "No value found for key %s in hash\n", key_string);
            test_monkey_object(actual_value, expected_value);
            free(key_string);
        }
    }
    cm_array_list_free(expected_keys);
    cm_array_list_free(actual_keys);
}
