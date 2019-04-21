#ifndef OBJECT_H
#define OBJECT_H

#include <stdbool.h>
#include "ast.h"
#include "environment.h"


typedef enum monkey_object_type {
    MONKEY_INT,
    MONKEY_BOOL,
    MONKEY_NULL,
    MONKEY_RETURN_VALUE,
    MONKEY_ERROR,
    MONKEY_FUNCTION,
    MONKEY_STRING,
    MONKEY_BUILTIN
} monkey_object_type;

static const char *type_names[] = {
    "INTEGER",
    "BOOLEAN",
    "NULL",
    "RETURN_VALUE",
    "MONKEY_ERROR",
    "FUNCTION",
    "STRING",
    "BUILTIN"
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

typedef struct monkey_return_value_t {
    monkey_object_t object;
    monkey_object_t *value;
} monkey_return_value_t;

typedef struct monkey_error_t {
    monkey_object_t object;
    char *message;
} monkey_error_t;

typedef struct monkey_function_t {
    monkey_object_t object;
    cm_list *parameters; // list of identifiers
    block_statement_t *body;
    environment_t *env;
} monkey_function_t;

typedef struct monkey_string_t {
    monkey_object_t object;
    char *value;
    size_t length;
} monkey_string_t;

typedef monkey_object_t * (*builtin_fn) (cm_list *);

typedef struct monkey_builtin_t {
    monkey_object_t object;
    builtin_fn function;
} monkey_builtin_t;


monkey_int_t * create_monkey_int(long);
monkey_bool_t * create_monkey_bool(_Bool);
monkey_null_t * create_monkey_null(void);
monkey_bool_t *get_monkey_true(void);
monkey_object_t *copy_monkey_object(monkey_object_t *);
monkey_return_value_t *create_monkey_return_value(monkey_object_t *);
monkey_error_t *create_monkey_error(const char *, ...);
monkey_function_t *create_monkey_function(cm_list *, block_statement_t *, environment_t *);
monkey_string_t *create_monkey_string(const char *, size_t);
monkey_builtin_t *create_monkey_builtin(builtin_fn);
void free_monkey_object(void *);

#endif
