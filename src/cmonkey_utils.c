#include <assert.h>
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
    assert(list != NULL);
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

char *
long_to_string(long l)
{
    long rem;
    size_t str_size = l / 10 + 1;
    char *str = malloc(str_size + 1);
    size_t index = str_size;
    str[index--] = 0;
    while (l > 10) {
        rem = l % 10;
        l = l / 10;
        str[index--] = 48 + rem;
    }
    if (l > 0)
        str[index] = 48 + l;
    return str;
}