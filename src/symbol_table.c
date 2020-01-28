#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "symbol_table.h"
#include "cmonkey_utils.h"

symbol_table_t *
symbol_table_init(void)
{
    symbol_table_t *table;
    table = malloc(sizeof(*table));
    if (table == NULL)
        err(EXIT_FAILURE, "malloc failed");
    table->nentries = 0;
    table->store = cm_hash_table_init(string_hash_function, string_equals,
            free, free_symbol);
    table->outer = NULL;
    return table;
}

symbol_table_t *
enclosed_symbol_table_init(symbol_table_t *outer)
{
    symbol_table_t *table = symbol_table_init();
    table->outer = outer;
    return table;
}

symbol_t *
symbol_define(symbol_table_t *table, char *name)
{
    symbol_scope_t scope = table->outer == NULL? GLOBAL: LOCAL;
    symbol_t *s = symbol_init(name, scope, table->nentries++);
    char *n = strdup(name);
    if (n == NULL)
        err(EXIT_FAILURE, "malloc failed");
    cm_hash_table_put(table->store, n, s);
    return s;
}

symbol_t *
symbol_define_builtin(symbol_table_t *table, size_t index, char *name)
{
    symbol_t *s = symbol_init(name, BUILTIN, index);
    char *n = strdup(name);
    if (n == NULL)
        err(EXIT_FAILURE, "malloc failed");
    cm_hash_table_put(table->store, n, s)    ;
    return s;
}

symbol_t *
symbol_init(char *name, symbol_scope_t scope, uint16_t index)
{
    symbol_t *s;
    s = malloc(sizeof(*s));
    if (s == NULL)
        err(EXIT_FAILURE, "malloc failed");
    s->name = strdup(name);
    if (s->name == NULL)
        err(EXIT_FAILURE, "malloc failed");
    s->scope = scope;
    s->index = index;
    return s;
}

symbol_t *
symbol_resolve(symbol_table_t *table, char *name)
{
    if (table->store == NULL)
        return NULL;
    void *obj = cm_hash_table_get(table->store, name);
    if (obj == NULL && table->outer != NULL)
        return symbol_resolve(table->outer, name);
    return (symbol_t *) obj;
}

void
free_symbol(void *o)
{
    symbol_t *s = (symbol_t *) o;
    free(s->name);
    free(s);
}

void
free_symbol_table(symbol_table_t *table)
{
    cm_hash_table_free(table->store);
    free(table);
}