#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdint.h>
#include "cmonkey_utils.h"

typedef enum symbol_scope_t {
    GLOBAL,
    LOCAL,
    BUILTIN
} symbol_scope_t;

static char * scope_names [] = {
    "GLOBAL",
    "LOCAL"
};

#define get_scope_name(s) scope_names[s]

typedef struct symbol_t {
    char *name;
    symbol_scope_t scope;
    uint16_t index;
} symbol_t;

typedef struct symbol_table_t {
    struct symbol_table_t *outer;
    cm_hash_table *store;
    uint16_t nentries;
} symbol_table_t;

symbol_table_t *symbol_table_init(void);
symbol_table_t *enclosed_symbol_table_init(symbol_table_t *);
symbol_t *symbol_define(symbol_table_t *, char *);
symbol_t *symbol_define_builtin(symbol_table_t *, size_t, char *);
symbol_t *symbol_resolve(symbol_table_t *, char *);
void free_symbol_table(symbol_table_t *);
void free_symbol(void *);
symbol_t *symbol_init(char *, symbol_scope_t, uint16_t);

#endif
