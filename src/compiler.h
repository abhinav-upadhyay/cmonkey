#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "cmonkey_utils.h"
#include "opcode.h"
#include "object.h"

typedef struct emitted_instrucion_t {
    opcode_t opcode;
    size_t position;
} emitted_instrucion_t;

typedef struct compiler_t {
    instructions_t *instructions;
    cm_array_list *constants_pool;
    emitted_instrucion_t last_instruction;
    emitted_instrucion_t prev_instruction;
} compiler_t;

typedef struct bytecode_t {
    instructions_t *instructions;
    cm_array_list *constants_pool;
} bytecode_t;

typedef enum compiler_error_code {
    COMPILER_ERROR_NONE,
    COMPILER_UNKNOWN_OPERATOR
} compiler_error_code;

typedef struct compiler_error_t {
    compiler_error_code code;
    char *msg;
} compiler_error_t;

static const char *compiler_errors[] = {
    "COMPILER_ERROR_NONE",
    "COMPILER_UNKNOWN_OPERATOR"
};


#define get_compiler_error(e) compiler_errors[e]

compiler_t *compiler_init(void);
void compiler_free(compiler_t *);
compiler_error_t compile(compiler_t *, node_t *);
bytecode_t *get_bytecode(compiler_t *);
void bytecode_free(bytecode_t *);

#endif