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

#include <assert.h>
#include <endian.h>
#include <err.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmonkey_utils.h"

void *
cm_list_get_at(cm_list *list, size_t index)
{
    cm_list_node *node = list->head;
    size_t i = 0;
    while (node != NULL) {
        if (i == index)
            return node->data;
        i++;
        node = node->next;
    }
    return NULL;
}

cm_list *
cm_list_init(void)
{
    cm_list *list;
    list = malloc(sizeof(*list));
    if (list == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    return list;
}

int
cm_list_add(cm_list *list, void *data)
{
    cm_list_node *node = malloc(sizeof(*node));
    if (node == NULL)
        return 0;
    node->data = data;
    node->next = NULL;
    list->length++;
    if (list->head == NULL) {
        list->head = node;
        list->tail = node;
        return 1;
    }

    list->tail->next = node;
    list->tail = node;
    return 1;
}

void
cm_list_free(cm_list *list, void (*free_data) (void *))
{
    if (list == NULL)
        return;
    cm_list_node *list_node = list->head;
    cm_list_node *temp_node = list_node;
    while (list_node != NULL) {
        if (free_data)
            free_data(list_node->data);
        // else
            // free(list_node->data);
        temp_node = list_node->next;
        free(list_node);
        list_node = temp_node;
    }
    free(list);
}

void *
cm_list_get(cm_list *list, void *key, _Bool (*cmp) (void *, void *))
{
    cm_list_node *node = list->head;
    while (node) {
        if (cmp(node->data, key))
            return node->data;
        node = node->next;
    }
    return NULL;
}


static
size_t calculate_string_size(long l)
{
    size_t size = 0;
    if (l < 0) {
        size++;
        l = -1 * l;
    }
    while (l >= 10) {
        l /= 10;
        size++;
    }
    return size + 1;
}

char *
long_to_string(long l)
{
    long rem;
    size_t str_size = calculate_string_size(l) + 1;
    char *str = malloc(str_size + 1);
    size_t index = str_size;
    _Bool is_negative = l < 0;
    if (is_negative)
        l = -1 * l;
    str[--index] = 0;
    while (l >= 10) {
        rem = l % 10;
        l /= 10;
        str[--index] = 48 + rem;
    }
    str[--index] = 48 + l;
    if (is_negative)
        str[--index] = '-';
    return str;
}

const char *
bool_to_string(_Bool value)
{
    if (value)
        return "true";
    return "false";
}

cm_hash_table *
cm_hash_table_init(size_t (*hash_func)(void *),
    _Bool (*keyequals) (void *, void *),
    void (*free_key) (void *),
    void (*free_value) (void *))
{
    cm_hash_table *table;
    table = malloc(sizeof(*table));
    if (table == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    table->hash_func = hash_func;
    table->keyequals = keyequals;
    table->free_key = free_key;
    table->free_value = free_value;
    table->table_size = INITIAL_HASHTABLE_SIZE;
    table->table = calloc(table->table_size, sizeof(*table->table));
    table->used_slots = cm_array_list_init(table->table_size, free);
    if (table->table == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    table->nkeys = 0;
    return table;
}

static cm_hash_entry *
find_entry(cm_list *list, cm_hash_entry *other_entry, _Bool (*keyequals) (void *, void *))
{
    cm_list_node *node = list->head;
    while (node) {
        cm_hash_entry *entry = (node->data);
        if (keyequals(entry->key, other_entry->key))
            return entry;
        node = node->next;
    }
    return NULL;
}


void
cm_hash_table_put(cm_hash_table *hash_table, void *key, void *value)
{
    cm_hash_entry *entry = NULL;
    size_t index = hash_table->hash_func(key) % hash_table->table_size;
    cm_list *entry_list = hash_table->table[index];
    if (entry_list == NULL) {
        entry_list = cm_list_init();
        hash_table->table[index] = entry_list;
        size_t *used_slot = malloc(sizeof(*used_slot));
        if (used_slot == NULL)
            errx(EXIT_FAILURE, "malloc failed");
        *used_slot = index;
        cm_array_list_add(hash_table->used_slots, used_slot);
        //TODO: resize when nentries == table size
    } else {
        cm_hash_entry temp_entry = {key, NULL};
        entry = find_entry(entry_list, &temp_entry, hash_table->keyequals);
    }

    if (entry == NULL) {
        entry = malloc(sizeof*entry);
        if (entry == NULL)
            errx(EXIT_FAILURE, "malloc failed");
        entry->key = key;
        entry->value = value;
        hash_table->nkeys++;
        cm_list_add(entry_list, entry);
    } else {
        if (hash_table->free_value)
            hash_table->free_value(entry->value);
        if (hash_table->free_key)
            hash_table->free_key(entry->key);
        entry->value = value;
        entry->key = key;
    }
}

void *
cm_hash_table_get(cm_hash_table *hash_table, void *key)
{
    size_t index = hash_table->hash_func(key) % hash_table->table_size;
    cm_list *entry_list = hash_table->table[index];
    cm_hash_entry *entry;
    if (entry_list == NULL)
        return NULL;
    cm_list_node *list_node = entry_list->head;
    while (list_node) {
        entry = (cm_hash_entry *) list_node->data;
        if (hash_table->keyequals(entry->key, key))
            return entry->value;
        list_node = list_node->next;
    }
    return NULL;
}

cm_array_list *
cm_hash_table_get_keys(cm_hash_table *hash_table)
{
    if (hash_table->nkeys == 0)
        return NULL;
    cm_array_list *keys_list = cm_array_list_init(hash_table->nkeys, NULL);
    for (size_t i = 0; i < hash_table->table_size; i++) {
        cm_list *bucket = hash_table->table[i];
        if (bucket == NULL)
            continue;
        cm_list_node *node = bucket->head;
        while (node != NULL) {
            cm_hash_entry *entry = (cm_hash_entry *) node->data;
            node = node->next;
            cm_array_list_add(keys_list, (void *) entry->key);
        }
    }
    return keys_list;
}

cm_array_list *
cm_hash_table_get_values(cm_hash_table *hash_table)
{
    cm_array_list *values_list = cm_array_list_init(hash_table->nkeys, NULL);
    for (size_t i = 0; i < hash_table->table_size; i++) {
        cm_list *bucket = hash_table->table[i];
        if (bucket == NULL)
            continue;
        cm_list_node *node = bucket->head;
        while (node != NULL) {
            cm_hash_entry *entry = (cm_hash_entry *) node->data;
            node = node->next;
            cm_array_list_add(values_list, (void *) entry->value);
        }
    }
    return values_list;
}

cm_hash_table *
cm_hash_table_copy(cm_hash_table *src, void * (*key_copy) (void *), void * (*value_copy) (void *))
{
    cm_hash_table *copy = cm_hash_table_init(src->hash_func,
        src->keyequals, src->free_key, src->free_value);
    for (size_t i = 0; i < src->used_slots->length; i++) {
        size_t *index = (size_t *) src->used_slots->array[i];
        cm_list *entry_list = src->table[*index];
        cm_list_node *entry_node = entry_list->head;
        while (entry_node != NULL) {
            cm_hash_entry *entry = (cm_hash_entry *) entry_node->data;
            void *key = entry->key;
            void *value = entry->value;
            cm_hash_table_put(copy, key_copy(key), value_copy(value));
            entry_node = entry_node->next;
        }
    }
    return copy;
}

size_t
string_hash_function(void *key)
{
    unsigned long hash = 5381;
    char *str = (char *) key;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

_Bool
string_equals(void *key1, void *key2)
{
    char *strkey1 = (char *) key1;
    char *strkey2 = (char *) key2;
    return strcmp(strkey1, strkey2) == 0;
}

static void
free_entry_list(cm_hash_table *table, cm_list *entry_list)
{
    cm_list_node *node = entry_list->head;
    cm_list_node *temp;
    while (node != NULL) {
        cm_hash_entry *entry = (cm_hash_entry *) node->data;
        if (table->free_key != NULL) {
            table->free_key(entry->key);
        }
        if (table->free_value != NULL) {
            table->free_value(entry->value);
        }
        free(entry);
        temp = node->next;
        free(node);
        node = temp;
    }
    free(entry_list);
}

void
cm_hash_table_free(cm_hash_table *table)
{
    for (size_t i = 0; i < table->table_size; i++) {
        cm_list *entry_list = table->table[i];
        if (entry_list != NULL) {
            free_entry_list(table, entry_list);
        }
    }
    free(table->table);
    cm_array_list_free(table->used_slots);
    free(table);
}

cm_array_list *
cm_array_list_init_size_t(size_t init_size, size_t count, ...)
{
    va_list ap;
    va_start(ap, count);
    cm_array_list *list = cm_array_list_init(init_size, NULL);
    for (size_t i = 0; i < count; i++) {
        cm_array_list_add(list, (void *) va_arg(ap, size_t));
    }
    return list;
}

cm_array_list *
cm_array_list_init(size_t init_size, void (*free_func) (void *))
{
    cm_array_list *list;
    list = malloc(sizeof(*list));
    if (list == NULL)
        err(EXIT_FAILURE, "malloc failed");
    list->array = calloc(init_size, sizeof(*list->array));
    if (list->array == NULL)
        err(EXIT_FAILURE, "malloc failed");
    for (size_t i = 0; i < init_size; i++)
        list->array[i] = NULL;
    list->array_size = init_size;
    list->length = 0;
    list->free_func = free_func;
    return list;
}

int
cm_array_list_add(cm_array_list *list, void *value)
{
    if (list->length == list->array_size) {
        list->array_size = list->array_size * 2 + 1;
        list->array = reallocarray(list->array, list->array_size, sizeof(*list->array));
        for (size_t i = list->length; i < list->array_size; i++)
            list->array[i] = NULL;
        if (list->array == NULL)
            err(EXIT_FAILURE, "malloc failed");
    }
    list->array[list->length++] = value;
    return 1;
}

static void
swap(void **array, size_t i, size_t j)
{
    void *tmp = array[i];
    array[i] = array[j];
    array[j] = tmp;
}

static size_t
partition(void **array, size_t lo, size_t hi, int (*cmp_func) (const void *, const void *))
{
    void *pivot = array[hi];
    size_t i = lo;
    for (size_t j = lo; j < hi; j++) {
        if (cmp_func(array[j], pivot) < 0) {
            swap(array, i, j);
            i++;
        }
    }
    swap(array, i, hi);
    return i;
}

static void
_qsort(void **array, size_t lo, size_t hi, int (*cmp_func) (const void *, const void *))
{
    if (array == NULL)
        return;
    if (lo == 0 && hi == 0)
        return;
    size_t pivot;
    if (lo < hi) {
        pivot = partition(array, lo, hi, cmp_func);
        _qsort(array, lo, pivot > 0? pivot - 1: pivot, cmp_func);
        _qsort(array, pivot + 1, hi, cmp_func);
    }
}

void
cm_array_list_sort(cm_array_list *list, size_t elem_size, int (*cmp_func) (const void *, const void *))
{
    if (list && list->length > 1)
        _qsort((list->array), 0, list->length - 1, cmp_func);
}

int
cm_array_list_add_at(cm_array_list *list, size_t index, void *value)
{
    if (index > list->length - 1)
        return 0;
    if (list->free_func != NULL)
        list->free_func(list->array[index]);
    list->array[index] = value;
    return 1;
}

// void *
// cm_array_list_get(cm_array_list *list, size_t index)
// {
//     if (index > list->length - 1)
//         return NULL;

//     return list->array[index];
// }

void *
cm_array_list_first(cm_array_list *list)
{
    return cm_array_list_get(list, 0);
}

void *cm_array_list_last(cm_array_list *list)
{
    return cm_array_list_get(list, list->length - 1);
}

void
cm_array_list_remove(cm_array_list *list, size_t index)
{
    cm_array_list *copy = cm_array_list_init(list->length - 1, list->free_func);
    for (size_t i = 0; i < list->length; i++) {
        if (i == index) {
            list->free_func(list->array[i]);
            continue;
        }
        cm_array_list_add(copy, list->array[i]);
    }

    free(list->array);
    list->array = copy->array;
    list->length = copy->length;
    list->array_size = copy->array_size;
    free(copy);
}

void
cm_array_list_free2(cm_array_list *list, void (*free_func)(void *))
{
    if (free_func != NULL) {
        for (size_t i = 0; i < list->length; i++)
            free_func(list->array[i]);
    }
    free(list->array);
    free(list);
}

void
cm_array_list_free(cm_array_list *list)
{
    if (list == NULL)
        return;
    cm_array_list_free2(list, list->free_func);
}

char *
cm_array_string_list_join(cm_array_list *list, const char *delim)
{
    char *string = NULL;
    char *temp = NULL;
    int ret;
    if (list == NULL)
        return NULL;

    for (size_t i = 0; i < list->length; i++) {
        if (string == NULL) {
            ret = asprintf(&temp, "%s", (char *) list->array[i]);
            if (ret == -1)
                err(EXIT_FAILURE, "malloc failed");
        } else {
            ret = asprintf(&temp, "%s%s%s", string, delim, (char *) list->array[i]);
            if (ret == -1)
                err(EXIT_FAILURE, "malloc failed");
            free(string);
        }
        string = temp;
        temp = NULL;
    }
    return string;
}

cm_array_list *
cm_array_list_copy(cm_array_list *list, void * (*copy_func) (void *))
{
    cm_array_list *copy = cm_array_list_init(list->length, list->free_func);
    for (size_t i = 0; i < list->length; i++) {
        cm_array_list_add(copy, copy_func(list->array[i]));
    }
    return copy;
}

size_t
int_hash_function(void *data)
{
    long *key = (long *) data;
    return *key * 2654435761 % (4294967296);
}

_Bool
int_equals(void *key1, void *key2)
{
    long *lkey1 = (long *) key1;
    long *lkey2 = (long*) key2;
    return *lkey1 == *lkey2;
}

size_t
pointer_hash_function(void *data)
{
    long key = (long) data;
    return key * 2654435761 % (4294967296);
}

_Bool
pointer_equals(void *key1, void*key2)
{
    return key1 == key2;
}

size_t *
create_size_t_array(size_t count, ...)
{
    va_list ap;
    va_start(ap, count);
    size_t *array;
    array = malloc(sizeof(*array) * count);
    if (array == NULL)
        err(EXIT_FAILURE, "malloc failed");
    for (size_t i = 0; i < count; i++)
        array[i] = (size_t) va_arg(ap, size_t);
    va_end(ap);
    return array;
}

uint8_t *
create_uint8_array(size_t count, ...)
{
    va_list ap;
    va_start(ap, count);
    uint8_t *array;
    array = malloc(sizeof(*array) * count);
    if (array == NULL)
        err(EXIT_FAILURE, "malloc failed");
    for (size_t i = 0; i < count; i++)
        array[i] = va_arg(ap, int);
    va_end(ap);
    return array;
}

static size_t
power_ceil(size_t x)
{
    size_t power = 1;
    while (x >>= 1)
        power++;
    return power;
}

uint8_t *
size_t_to_uint8_be(size_t val, size_t nbytes)
{
    uint8_t *array;
    array = malloc(sizeof(*array) * nbytes);
    if (array == NULL)
        err(EXIT_FAILURE, "malloc failed");
    uint64_t newval = htobe64(val); // to make sure we work on both BE and LE archs
    uint8_t *x = (uint8_t *) &newval;
    int j = nbytes - 1;
    int i = sizeof(val) - 1;
    while (j >= 0)
        array[j--] = x[i--];
    return array;
}

size_t
be_to_size_t(uint8_t *bytes, size_t bytes_count)
{
    uint8_t *barray;
    uint16_t two_bytes = 0;
    // if (bytes_count != 2)
        // err(EXIT_FAILURE, "We don't support operands of %zu bytes width", bytes_count);

    barray = (uint8_t *) &two_bytes;
    if (bytes[0] == 0)
        return (size_t) bytes[1] + bytes[0];
    else
        return (size_t) (bytes[0] << 8) + bytes[1];
}

cm_stack *
cm_stack_init(void)
{
    cm_stack *stack;
    stack = malloc(sizeof(*stack));
    if (stack == NULL)
        err(EXIT_FAILURE, "malloc failed");
    stack->list = cm_list_init();
    return stack;
}

void
cm_stack_push(cm_stack *stack, void *obj)
{
    cm_list_node *node;
    node = malloc(sizeof(*node));
    if (node == NULL)
        err(EXIT_FAILURE, "malloc failed");
    node->data = obj;
    node->next = stack->list->head;
    stack->list->head = node;
}

void *
cm_stack_pop(cm_stack *stack)
{
    void *head = stack->list->head;
    stack->list->head = stack->list->head->next;
    return head;
}