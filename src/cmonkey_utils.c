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

#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmonkey_utils.h"

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
        else
            free(list_node->data);
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
    _Bool (*keycmp) (void *, void *),
    void (*free_key) (void *),
    void (*free_value) (void *))
{
    cm_hash_table *table;
    table = malloc(sizeof(*table));
    if (table == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    table->hash_func = hash_func;
    table->keycmp = keycmp;
    table->free_key = free_key;
    table->free_value = free_value;
    table->table_size = INITIAL_HASHTABLE_SIZE;
    table->table = calloc(table->table_size, sizeof(*table->table));
    if (table->table == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    table->nentries = 0;
    table->nkeys = 0;
    return table;
}

static _Bool
entry_cmp(void *e1, void *e2)
{
    cm_hash_entry *entry1 = (cm_hash_entry *) e1;
    cm_hash_entry *entry2 = (cm_hash_entry *) e2;
    return string_keycmp(entry1->key, entry2->key);
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
        hash_table->nentries++;
        //TODO: resize when nentries == table size
    } else {
        cm_hash_entry temp_entry = {key, NULL};
        entry = cm_list_get(entry_list, &temp_entry, entry_cmp);
    }

    if (entry == NULL) {
        entry = malloc(sizeof*entry);
        if (entry == NULL)
            errx(EXIT_FAILURE, "malloc failed");
    }
    entry->key = key;
    entry->value = value;
    hash_table->nkeys++;
    cm_list_add(entry_list, entry);
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
        if (hash_table->keycmp(entry->key, key))
            return entry->value;
        list_node = list_node->next;
    }
    return NULL;
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
string_keycmp(void *key1, void *key2)
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
    free(table);
}

cm_array_list *
cm_array_list_init(size_t init_size, void (*free_func) (void *))
{
    cm_array_list *list;
    list = malloc(sizeof(*list));
    if (list == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    list->array = calloc(init_size, sizeof(*list->array));
    if (list->array == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    list->array_size = init_size;
    list->length = 0;
    list->free_func = free_func;
    return list;
}

int
cm_array_list_add(cm_array_list *list, void *value)
{
    if (list->length == list->array_size) {
        list->array_size *= 2;
        list->array = reallocarray(list->array, list->array_size, sizeof(*list->array));
        if (list->array == NULL)
            errx(EXIT_FAILURE, "malloc failed");
    }
    list->array[list->length++] = value;
    return 1;
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

void *
cm_array_list_get(cm_array_list *list, size_t index)
{
    if (index > list->length - 1)
        return NULL;

    return list->array[index];
}

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
cm_array_list_free(cm_array_list *list)
{
    if (list->free_func != NULL) {
        for (size_t i = 0; i < list->length; i++) {
            list->free_func(list->array[i]);
        }
    }
    free(list->array);
    free(list);
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
                errx(EXIT_FAILURE, "malloc failed");
        } else {
            ret = asprintf(&temp, "%s%s%s", string, delim, list->array[i]);
            if (ret == -1)
                errx(EXIT_FAILURE, "malloc failed");
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