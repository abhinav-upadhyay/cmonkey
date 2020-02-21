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

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <stdint.h>
#include "cmonkey_utils.h"

typedef enum symbol_scope_t {
    GLOBAL,
    LOCAL,
    BUILTIN,
    FREE,
    FUNCTION_SCOPE
} symbol_scope_t;

static char * scope_names [] = {
    "GLOBAL",
    "LOCAL",
    "BUILTIN",
    "FREE",
    "FUNCTION"
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
    cm_array_list *free_symbols;
    uint16_t nentries;
} symbol_table_t;

symbol_table_t *symbol_table_init(void);
symbol_table_t *enclosed_symbol_table_init(symbol_table_t *);
symbol_t *symbol_define(symbol_table_t *, char *);
symbol_t *symbol_define_builtin(symbol_table_t *, size_t, char *);
symbol_t *symbol_define_function(symbol_table_t *, char *);
symbol_t *symbol_resolve(symbol_table_t *, const char *);
void free_symbol_table(symbol_table_t *);
void free_symbol(void *);
symbol_t *symbol_init(char *, symbol_scope_t, uint16_t);

#endif
