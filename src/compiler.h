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