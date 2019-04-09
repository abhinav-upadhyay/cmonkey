#ifndef OBJECT_H
#define OBJECT_H

#include <stdbool.h>

typedef enum monkey_object_type {
    MONKEY_INT,
    MONKEY_BOOL,
    MONKEY_NULL
} monkey_object_type;

static const char *type_names[] = {
    "INTEGER",
    "BOOL",
    "NULL"
};

#define get_type_name(type) type_names[type]

typedef struct monkey_object_t {
    monkey_object_type type;
    char * (*inspect) (struct monkey_object_t *);
} monkey_object_t;

typedef struct monkey_int_t {
    monkey_object_t object;
    long value;
} monkey_int_t;

typedef struct monkey_bool_t {
    monkey_object_t object;
    _Bool value;
} monkey_bool_t;

typedef struct monkey_null_t {
    monkey_object_t object;
} monkey_null_t;

monkey_int_t * create_monkey_int(long);
monkey_bool_t * create_monkey_bool(_Bool);
monkey_null_t * create_monkey_null(void);
void free_monkey_object(monkey_object_t *);

#endif
