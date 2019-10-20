#include <err.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "cmonkey_utils.h"
#include "compiler.h"
#include "object.h"
#include "opcode.h"

#define CONSTANTS_POOL_INIT_SIZE 16

static size_t
add_instructions(compiler_t *compiler, instructions_t *ins)
{
    size_t new_ins_pos = compiler->instructions->length;
    concat_instructions(compiler->instructions, ins);
    instructions_free(ins);
    return new_ins_pos;
}

static size_t
emit(compiler_t *compiler, opcode_t op, ...)
{
    va_list ap;
    va_start(ap, op);
    instructions_t *ins = vinstruction_init(op, ap);
    size_t new_ins_pos = add_instructions(compiler, ins);
    va_end(ap);
    return new_ins_pos;
}

compiler_t *
compiler_init(void)
{
    compiler_t *compiler;
    compiler = malloc(sizeof(*compiler));
    if (compiler == NULL)
        err(EXIT_FAILURE, "malloc failed");
    compiler->instructions = malloc(sizeof(*compiler->instructions));
    if (compiler->instructions == NULL)
        err(EXIT_FAILURE, "malloc failed");
    compiler->instructions->bytes = NULL;
    compiler->instructions->length = 0;
    compiler->instructions->size = 0;
    compiler->constants_pool = NULL;
    return compiler;
}

void
compiler_free(compiler_t *compiler)
{
    instructions_free(compiler->instructions);
    if (compiler->constants_pool)
        cm_array_list_free(compiler->constants_pool);
    free(compiler);
}

void
bytecode_free(bytecode_t *bytecode)
{
    free(bytecode);
}

static size_t
add_constant(compiler_t *compiler, monkey_object_t *obj)
{
    if (compiler->constants_pool == NULL)
        compiler->constants_pool = cm_array_list_init(CONSTANTS_POOL_INIT_SIZE, free_monkey_object);
    cm_array_list_add(compiler->constants_pool, obj);
    return compiler->constants_pool->length - 1;
}

static compiler_error_t
compile_expression_node(compiler_t *compiler, expression_t *expression_node)
{
    compiler_error_t error;
    infix_expression_t *infix_exp;
    integer_t *int_exp;
    monkey_int_t *int_obj;
    switch (expression_node->expression_type) {
    case INFIX_EXPRESSION:
        infix_exp = (infix_expression_t *) expression_node;
        error = compile(compiler, (node_t *) infix_exp->left);
        if (error != COMPILER_ERROR_NONE)
            return error;
        error = compile(compiler, (node_t *) infix_exp->right);
        if (error != COMPILER_ERROR_NONE)
            return error;
        if (strcmp(infix_exp->operator, "+") == 0)
            emit(compiler, OPADD);
        else
            return COMPILER_UNKNOWN_OPERATOR;
        break;
    case INTEGER_EXPRESSION:
        int_exp = (integer_t *) expression_node;
        int_obj = create_monkey_int(int_exp->value);
        size_t constant_idx = add_constant(compiler, (monkey_object_t *) int_obj);
        emit(compiler, OPCONSTANT, constant_idx);
        return COMPILER_ERROR_NONE;
    default:
        return COMPILER_ERROR_NONE;
    }
    return COMPILER_ERROR_NONE;
}

static compiler_error_t
compile_statement_node(compiler_t *compiler, statement_t *statement_node)
{
    compiler_error_t error;
    expression_statement_t *expression_stmt;
    switch (statement_node->statement_type) {
    case EXPRESSION_STATEMENT:
        expression_stmt = (expression_statement_t *) statement_node;
        error = compile_expression_node(compiler, expression_stmt->expression);
        if (error != COMPILER_ERROR_NONE)
            return error;
    default:
        return COMPILER_ERROR_NONE;
    }
    return COMPILER_ERROR_NONE;
}

compiler_error_t
compile(compiler_t *compiler, node_t *node)
{
    program_t *program;
    expression_t *expression_node;
    statement_t *statement_node;
    compiler_error_t error;
    size_t i;
    switch (node->type) {
    case PROGRAM:
        program = (program_t *) node;
        for (i = 0; i < program->nstatements; i++) {
            error = compile(compiler, (node_t *) program->statements[i]);
            if (error != COMPILER_ERROR_NONE)
                return error;
        }
        break;
    case STATEMENT:
        statement_node = (statement_t *) node;
        error = compile_statement_node(compiler, statement_node);
        if (error != COMPILER_ERROR_NONE)
            return error;
        break;
    case EXPRESSION:
        expression_node = (expression_t *) node;
        error = compile_expression_node(compiler, expression_node);
        if (error != COMPILER_ERROR_NONE)
            return error;
        break;
    default:
        return COMPILER_ERROR_NONE;
    }
    return COMPILER_ERROR_NONE;
}

bytecode_t *
get_bytecode(compiler_t *compiler)
{
    bytecode_t *bytecode;
    bytecode = malloc(sizeof(*bytecode));
    bytecode->instructions = compiler->instructions;
    bytecode->constants_pool = compiler->constants_pool;
    return bytecode;
}