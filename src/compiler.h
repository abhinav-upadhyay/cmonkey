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

#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "cmonkey_utils.h"
#include "opcode.h"
#include "object.h"
#include "symbol_table.h"

typedef struct emitted_instrucion_t {
    opcode_t opcode;
    size_t position;
} emitted_instrucion_t;

typedef struct compilation_scope_t {
    instructions_t *instructions;
    emitted_instrucion_t last_instruction;
    emitted_instrucion_t prev_instruction;
} compilation_scope_t;


typedef struct compiler_t {
    cm_array_list *constants_pool;
    symbol_table_t *symbol_table;
    cm_array_list *scopes;
    size_t scope_index;
} compiler_t;

typedef struct bytecode_t {
    instructions_t *instructions;
    cm_array_list *constants_pool;
} bytecode_t;

typedef enum compiler_error_code {
    COMPILER_ERROR_NONE,
    COMPILER_UNKNOWN_OPERATOR,
    COMPILER_UNDEFINED_VARIABLE
} compiler_error_code;

typedef struct compiler_error_t {
    compiler_error_code code;
    char *msg;
} compiler_error_t;

static const char *compiler_errors[] = {
    "COMPILER_ERROR_NONE",
    "COMPILER_UNKNOWN_OPERATOR",
    "COMPILER_UNDEFINED_VARIABLE"
};


#define get_compiler_error(e) compiler_errors[e]

compiler_t *compiler_init(void);
compiler_t *compiler_init_with_state(symbol_table_t *, cm_array_list *);
void compiler_free(compiler_t *);
compiler_error_t compile(compiler_t *, node_t *);
bytecode_t *get_bytecode(compiler_t *);
void bytecode_free(bytecode_t *);
symbol_table_t *symbol_table_copy(symbol_table_t *);
size_t emit(compiler_t *, opcode_t, ...);
void compiler_enter_scope(compiler_t *);
instructions_t *compiler_leave_scope(compiler_t *);
compilation_scope_t *scope_init(void);
void scope_free(compilation_scope_t *);
compilation_scope_t *get_top_scope(compiler_t *);
#endif