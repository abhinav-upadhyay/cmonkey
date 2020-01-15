/*-
 * Copyright (c) 2019 Abhinav Upadhyay <er.abhinav.upadhyay@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef OBJECT_H
#define OBJECT_H

#include <stdbool.h>
#include "ast.h"
#include "environment.h"
#include "opcode.h"

typedef enum monkey_object_type {
    MONKEY_INT,
    MONKEY_BOOL,
    MONKEY_NULL,
    MONKEY_RETURN_VALUE,
    MONKEY_ERROR,
    MONKEY_FUNCTION,
    MONKEY_STRING,
    MONKEY_BUILTIN,
    MONKEY_ARRAY,
    MONKEY_HASH,
    MONKEY_COMPILED_FUNCTION
} monkey_object_type;

static const char *type_names[] = {
    "INTEGER",
    "BOOLEAN",
    "NULL",
    "RETURN_VALUE",
    "MONKEY_ERROR",
    "FUNCTION",
    "STRING",
    "BUILTIN",
    "ARRAY",
    "HASH",
    "COMPILED_FUNCTION"
};

#define get_type_name(type) type_names[type]

typedef struct monkey_object_t {
    monkey_object_type type;
    char * (*inspect) (struct monkey_object_t *);
    size_t (*hash) (void *);
    _Bool (*equals) (void *, void *);
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

typedef struct monkey_compiled_fn_t {
    monkey_object_t object;
    instructions_t *instructions;
    size_t num_locals;
} monkey_compiled_fn_t;

typedef monkey_object_t * (*builtin_fn) (cm_list *);

typedef struct monkey_builtin_t {
    monkey_object_t object;
    builtin_fn function;
} monkey_builtin_t;

typedef struct monkey_array_t {
    monkey_object_t object;
    cm_array_list *elements;
} monkey_array_t;

typedef struct monkey_hash_t {
    monkey_object_t object;
    cm_hash_table *pairs;
} monkey_hash_t;

char *inspect(monkey_object_t *);
_Bool monkey_object_equals(void *, void *);
size_t monkey_object_hash(void *); // non-static for tests

static monkey_bool_t MONKEY_TRUE_OBJ = {{MONKEY_BOOL, inspect, monkey_object_hash, monkey_object_equals}, true};
static monkey_bool_t MONKEY_FALSE_OBJ = {{MONKEY_BOOL, inspect, monkey_object_hash, monkey_object_equals}, false};
static monkey_null_t MONKEY_NULL_OBJ = {{MONKEY_NULL, inspect, NULL, NULL}};

#define create_monkey_bool(val) ((val == true) ? (&MONKEY_TRUE_OBJ): (&MONKEY_FALSE_OBJ))
#define create_monkey_null() (&MONKEY_NULL_OBJ)


monkey_int_t * create_monkey_int(long);
monkey_bool_t *get_monkey_true(void);
monkey_object_t *copy_monkey_object(monkey_object_t *);
monkey_return_value_t *create_monkey_return_value(monkey_object_t *);
monkey_error_t *create_monkey_error(const char *, ...);
monkey_function_t *create_monkey_function(cm_list *, block_statement_t *, environment_t *);
monkey_string_t *create_monkey_string(const char *, size_t);
monkey_builtin_t *create_monkey_builtin(builtin_fn);
monkey_array_t *create_monkey_array(cm_array_list *);
monkey_hash_t *create_monkey_hash(cm_hash_table *);
monkey_compiled_fn_t *create_monkey_compiled_fn(instructions_t *, size_t);
void free_monkey_object(void *);

#endif
