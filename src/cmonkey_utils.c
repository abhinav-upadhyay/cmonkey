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

#include <stdbool.h>
#include <stdlib.h>

#include "cmonkey_utils.h"

cm_list *
cm_list_init(void)
{
    cm_list *list;
    list = malloc(sizeof(*list));
    if (list == NULL)
        return NULL;
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