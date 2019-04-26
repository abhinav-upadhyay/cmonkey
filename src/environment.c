#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "cmonkey_utils.h"
#include "environment.h"
#include "object.h"

static void
free_value(void *value)
{
    monkey_object_t *obj = (monkey_object_t *) value;
    free_monkey_object(obj);
}

environment_t *
create_env(void)
{
    cm_hash_table *table = cm_hash_table_init(
        string_hash_function,
        string_equals,
        free,
        free_value);
    environment_t *env;
    env = malloc(sizeof(*env));
    if (env == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    env->table = table;
    env->outer = NULL;
    return env;
}

environment_t *
create_enclosed_env(environment_t *outer)
{
    environment_t *env = create_env();
    env->outer = copy_env(outer);
    return env;
}

void
env_put(environment_t *env, char *name, void *value)
{
    cm_hash_table_put(env->table, (void *) name, value);
}

void *
env_get(environment_t *env, char *name)
{
    void *value;
    value = cm_hash_table_get(env->table, (void *) name);
    if (value != NULL)
        return value;
    
    if (env->outer != NULL)
        return env_get(env->outer, name);
    return NULL;
}

void
env_free(environment_t *env)
{
    if (env->outer != NULL) {
        env_free(env->outer);
    }
    cm_hash_table_free(env->table);
    free(env);
}

environment_t *
copy_env(environment_t *env)
{
    environment_t *new_env = create_env();
    for (size_t i = 0; i < env->table->table_size; i++) {
        cm_list *entry_list = env->table->table[i];
        if (entry_list == NULL)
            continue;
        cm_list_node *node = entry_list->head;
        while (node != NULL) {
            cm_hash_entry *entry = (cm_hash_entry *) node->data;
            char *key = (char *) entry->key;
            monkey_object_t *value = (monkey_object_t *) entry->value;
            env_put(new_env, strdup(key), copy_monkey_object(value));
            node = node->next;
        }
    }
    return new_env;
}
