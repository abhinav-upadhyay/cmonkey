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
    table->free_symbols = cm_array_list_init(8, NULL);
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
symbol_define_function(symbol_table_t *table, char *name)
{
    symbol_t *s = symbol_init(name, FUNCTION_SCOPE, 0);
    char *n = strdup(name);
    if (n == NULL)
        err(EXIT_FAILURE, "malloc failed");
    cm_hash_table_put(table->store, n, s);
    return s;
}

static symbol_t *
symbol_define_free(symbol_table_t *table, symbol_t *original)
{
    cm_array_list_add(table->free_symbols, original);
    symbol_t *sym = symbol_init(original->name, FREE, table->free_symbols->length - 1);
    cm_hash_table_put(table->store, strdup(original->name), sym);
    return sym;
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
symbol_resolve(symbol_table_t *table, const char *name)
{
    if (table->store == NULL)
        return NULL;
    void *obj = cm_hash_table_get(table->store, (void *) name);
    if (obj == NULL && table->outer != NULL) {
        symbol_t *sym = symbol_resolve(table->outer, name);
        if (sym == NULL)
            return NULL;
        if (sym->scope == GLOBAL || sym->scope == BUILTIN)
            return sym;
        return symbol_define_free(table, sym);
    }
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
    cm_array_list_free(table->free_symbols);
    free(table);
}