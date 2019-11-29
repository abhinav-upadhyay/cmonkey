#ifndef OBJECT_TEST_UTILS_H
#define OBJECT_TEST_UTILS_H
#include "object.h"

void test_monkey_object(monkey_object_t *, monkey_object_t *);
void test_null_object(monkey_object_t *);
void test_integer_object(monkey_object_t *, long);
void test_boolean_object(monkey_object_t *, _Bool);
void test_array_object(monkey_object_t *, monkey_object_t *);
void test_hash_object(monkey_object_t *, monkey_object_t *);
#endif