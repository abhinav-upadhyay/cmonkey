#ifndef CMONKEY_UTILS_H
#define CMONKEY_UTILS_H

typedef struct cm_list_node {
    void *data;
    struct cm_list_node *next;
} cm_list_node;

typedef struct cm_list {
    cm_list_node *head;
    cm_list_node *tail;
    size_t length;
} cm_list;


cm_list *cm_list_init(void);
int cm_list_add(cm_list *, void *);
void cm_list_free(cm_list *, void (*free_data) (void *));
#endif