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

#ifndef CMONKEY_UTILS_H
#define CMONKEY_UTILS_H

#include <stdbool.h>
#include <stdlib.h>

#define INITIAL_HASHTABLE_SIZE 64

typedef struct cm_list_node {
    void *data;
    struct cm_list_node *next;
} cm_list_node;

typedef struct cm_list {
    cm_list_node *head;
    cm_list_node *tail;
    size_t length;
} cm_list;

typedef struct cm_array_list {
    void **array;
    size_t length;
    size_t array_size;
    void (*free_func) (void *);
} cm_array_list;

typedef struct cm_hash_entry {
    void *key;
    void *value;
} cm_hash_entry;

typedef struct cm_hash_table {
    cm_list **table;
    cm_array_list *used_slots;
    size_t table_size;
    size_t nkeys; // actual number of keys stored
    size_t (*hash_func) (void *);
    _Bool (*keyequals) (void *, void *);
    void (*free_key) (void *);
    void (*free_value) (void *);
} cm_hash_table;


cm_list *cm_list_init(void);
int cm_list_add(cm_list *, void *);
void cm_list_free(cm_list *, void (*free_data) (void *));
void *cm_list_get(cm_list *, void *, _Bool (*cmp) (void *, void *));

cm_array_list * cm_array_list_init(size_t, void (*free_func) (void *));
int cm_array_list_add(cm_array_list *, void *);
int cm_array_list_add_at(cm_array_list *, size_t, void *);
void *cm_array_list_get(cm_array_list *, size_t);
void *cm_array_list_last(cm_array_list *);
void *cm_array_list_first(cm_array_list *);
void cm_array_list_remove(cm_array_list *, size_t);
void cm_array_list_free(cm_array_list *);
char *cm_array_string_list_join(cm_array_list *, const char *);
cm_array_list *cm_array_list_copy(cm_array_list *, void * (*copy_func) (void *));

char *long_to_string(long);
const char *bool_to_string(_Bool);

cm_hash_table *cm_hash_table_init(
    size_t (*hash_func)(void *),
    _Bool (*keyequals) (void *, void *),
    void (*free_key) (void *),
    void (*free_value) (void *));
void cm_hash_table_put(cm_hash_table *, void *, void *);
void *cm_hash_table_get(cm_hash_table *, void *);
void cm_hash_table_free(cm_hash_table *);
cm_hash_table *cm_hash_table_copy(cm_hash_table *, void * (*key_copy) (void *), void * (*value_copy) (void *));
size_t string_hash_function(void *);
_Bool string_equals(void *, void *);
size_t int_hash_function(void *);
_Bool int_equals(void *, void *);
size_t pointer_hash_function(void *);
_Bool pointer_equals(void *, void*);
#endif