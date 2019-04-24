#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "cmonkey_utils.h"

typedef struct environment_t {
    cm_hash_table *table;
    struct environment_t *outer;
} environment_t;

void *env_get(environment_t *, char *);
void env_put(environment_t *, char *, void *);
environment_t *create_env(void);
environment_t *create_enclosed_env(environment_t *);
void env_free(environment_t *);
environment_t *copy_env(environment_t *);
#endif