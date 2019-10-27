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

static char *
get_err_msg(const char *s, ...)
{
    char *msg = NULL;
    va_list ap;
    va_start(ap, s);
    int retval = vasprintf(&msg, s, ap);
    va_end(ap);
    if (retval == -1)
        err(EXIT_FAILURE, "malloc failed");
    return msg;
}

static compiler_error_t
compile_expression_node(compiler_t *compiler, expression_t *expression_node)
{
    compiler_error_t error;
    compiler_error_t none_error = {COMPILER_ERROR_NONE, NULL};
    infix_expression_t *infix_exp;
    prefix_expression_t *prefix_exp;
    integer_t *int_exp;
    boolean_expression_t *bool_exp;
    monkey_int_t *int_obj;
    monkey_bool_t *bool_obj;
    switch (expression_node->expression_type) {
    case INFIX_EXPRESSION:
        infix_exp = (infix_expression_t *) expression_node;
        if (strcmp(infix_exp->operator, "<") == 0) {
            error = compile(compiler, (node_t *) infix_exp->right);
            if (error.code != COMPILER_ERROR_NONE)
                return error;
            error = compile(compiler, (node_t *) infix_exp->left);
            if (error.code != COMPILER_ERROR_NONE)
                return error;
            emit(compiler, OPGREATERTHAN);
            break;
        }
        error = compile(compiler, (node_t *) infix_exp->left);
        if (error.code != COMPILER_ERROR_NONE)
            return error;
        error = compile(compiler, (node_t *) infix_exp->right);
        if (error.code != COMPILER_ERROR_NONE)
            return error;
        if (strcmp(infix_exp->operator, "+") == 0)
            emit(compiler, OPADD);
        else if (strcmp(infix_exp->operator, "-") == 0)
            emit(compiler, OPSUB);
        else if (strcmp(infix_exp->operator, "*") == 0)
            emit(compiler, OPMUL);
        else if(strcmp(infix_exp->operator, "/") == 0)
            emit(compiler, OPDIV);
        else if (strcmp(infix_exp->operator, ">") == 0)
            emit(compiler, OPGREATERTHAN);
        else if (strcmp(infix_exp->operator, "==") == 0)
            emit(compiler, OPEQUAL);
        else if (strcmp(infix_exp->operator, "!=") == 0)
            emit(compiler, OPNOTEQUAL);
        else {
            error.code = COMPILER_UNKNOWN_OPERATOR;
            error.msg = get_err_msg("Unknown operator %s", infix_exp->operator);
            return error;
        }
        break;
    case PREFIX_EXPRESSION:
        prefix_exp = (prefix_expression_t *) expression_node;
        error = compile(compiler, (node_t *) prefix_exp->right);
        if (error.code != COMPILER_ERROR_NONE)
            return error;
        if (strcmp(prefix_exp->operator, "-") == 0)
            emit(compiler, OPMINUS);
        else if (strcmp(prefix_exp->operator, "!") == 0)
            emit(compiler, OPBANG);
        else {
            error.code = COMPILER_UNKNOWN_OPERATOR;
            error.msg = get_err_msg("Unknown operator %s", prefix_exp->operator);
            return error;
        }
        break;
    case INTEGER_EXPRESSION:
        int_exp = (integer_t *) expression_node;
        int_obj = create_monkey_int(int_exp->value);
        size_t constant_idx = add_constant(compiler, (monkey_object_t *) int_obj);
        emit(compiler, OPCONSTANT, constant_idx);
        return none_error;
    case BOOLEAN_EXPRESSION:
        bool_exp = (boolean_expression_t *) expression_node;
        bool_obj = create_monkey_bool(bool_exp->value);
        if (bool_obj->value)
            emit(compiler, OPTRUE);
        else
            emit(compiler, OPFALSE);
        return none_error;
    default:
        return none_error;
    }
    return none_error;
}

static compiler_error_t
compile_statement_node(compiler_t *compiler, statement_t *statement_node)
{
    compiler_error_t error;
    compiler_error_t none_error = {COMPILER_ERROR_NONE, NULL};
    expression_statement_t *expression_stmt;
    switch (statement_node->statement_type) {
    case EXPRESSION_STATEMENT:
        expression_stmt = (expression_statement_t *) statement_node;
        error = compile_expression_node(compiler, expression_stmt->expression);
        if (error.code != COMPILER_ERROR_NONE)
            return error;
        emit(compiler, OPPOP);
    default:
        return none_error;
    }
    return none_error;
}

compiler_error_t
compile(compiler_t *compiler, node_t *node)
{
    program_t *program;
    expression_t *expression_node;
    statement_t *statement_node;
    compiler_error_t error;
    compiler_error_t none_error = {COMPILER_ERROR_NONE, NULL};
    size_t i;
    switch (node->type) {
    case PROGRAM:
        program = (program_t *) node;
        for (i = 0; i < program->nstatements; i++) {
            error = compile(compiler, (node_t *) program->statements[i]);
            if (error.code != COMPILER_ERROR_NONE)
                return error;
        }
        break;
    case STATEMENT:
        statement_node = (statement_t *) node;
        error = compile_statement_node(compiler, statement_node);
        if (error.code != COMPILER_ERROR_NONE)
            return error;
        break;
    case EXPRESSION:
        expression_node = (expression_t *) node;
        error = compile_expression_node(compiler, expression_node);
        if (error.code != COMPILER_ERROR_NONE)
            return error;
        break;
    default:
        return none_error;
    }
    return none_error;
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