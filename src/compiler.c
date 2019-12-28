#include <err.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "cmonkey_utils.h"
#include "compiler.h"
#include "object.h"
#include "opcode.h"

#define CONSTANTS_POOL_INIT_SIZE 16

static instructions_t *
get_current_instructions(compiler_t *compiler)
{
    return get_top_scope(compiler)->instructions;
}

static _Bool
last_instruction_is(compiler_t *compiler, opcode_t opcode)
{
    if (get_current_instructions(compiler)->length == 0)
        return false;
    return get_top_scope(compiler)->last_instruction.opcode == opcode;
}

static void
_scope_free(void *scope)
{
    scope_free((compilation_scope_t *) scope);
}

static void
remove_last_instruction(compiler_t *compiler)
{
    compilation_scope_t *scope = get_top_scope(compiler);
    scope->instructions->length = scope->last_instruction.position;
    scope->last_instruction.opcode = scope->prev_instruction.opcode;
    scope->last_instruction.position = scope->prev_instruction.position;
}

static size_t
add_instructions(compiler_t *compiler, instructions_t *ins)
{
    compilation_scope_t *scope = get_top_scope(compiler);
    size_t new_ins_pos = scope->instructions->length;
    concat_instructions(scope->instructions, ins);
    instructions_free(ins);
    return new_ins_pos;
}

static void
set_last_instruction(compiler_t *compiler, opcode_t opcode, size_t pos)
{
    compilation_scope_t *scope = get_top_scope(compiler);
    scope->prev_instruction.opcode = scope->last_instruction.opcode;
    scope->prev_instruction.position = scope->last_instruction.position;
    scope->last_instruction.opcode = opcode;
    scope->last_instruction.position = pos;
}

static void
replace_instruction(compiler_t *compiler, size_t position, instructions_t *ins)
{
    compilation_scope_t *scope = get_top_scope(compiler);
    for (size_t i = 0; i < ins->length; i++)
        scope->instructions->bytes[position + i] = ins->bytes[i];
}

static void
change_operand(compiler_t *compiler, size_t op_pos, size_t operand)
{
    compilation_scope_t *scope = get_top_scope(compiler);
    opcode_t op = (opcode_t) scope->instructions->bytes[op_pos];
    instructions_t *new_ins = instruction_init(op, operand);
    replace_instruction(compiler, op_pos, new_ins);
    instructions_free(new_ins);
}

size_t
emit(compiler_t *compiler, opcode_t op, ...)
{
    va_list ap;
    va_start(ap, op);
    instructions_t *ins = vinstruction_init(op, ap);
    size_t new_ins_pos = add_instructions(compiler, ins);
    va_end(ap);
    set_last_instruction(compiler, op, new_ins_pos);
    return new_ins_pos;
}


compilation_scope_t *
scope_init()
{
    compilation_scope_t *scope;
    scope = malloc(sizeof(*scope));
    if (scope == NULL)
        err(EXIT_FAILURE, "malloc failed");
    scope->instructions = malloc(sizeof(*scope->instructions));
    if (scope->instructions == NULL)
        err(EXIT_FAILURE, "malloc failed");
    scope->instructions->bytes = NULL;
    scope->instructions->length = 0;
    scope->instructions->size = 0;
    return scope;
}

compiler_t *
compiler_init(void)
{
    compiler_t *compiler;
    compiler = malloc(sizeof(*compiler));
    if (compiler == NULL)
        err(EXIT_FAILURE, "malloc failed");
    compiler->constants_pool = NULL;
    compiler->symbol_table = symbol_table_init();
    compiler->scope_index = 0;
    compiler->scopes = cm_array_list_init(16, _scope_free);
    compilation_scope_t *main_scope = scope_init();
    cm_array_list_add(compiler->scopes, main_scope);
    return compiler;
}

static void *
_strdup(void *s)
{
    return (void *) strdup((char *) s);
}

static void *
_copy_monkey_object(void *obj)
{
    return copy_monkey_object((monkey_object_t *) obj);
}

static void *
_copy_symbol(void *obj)
{
    symbol_t *src = (symbol_t *) obj;
    symbol_t *new_symbol = symbol_init(src->name, src->scope, src->index);
    return new_symbol;
}

symbol_table_t *
symbol_table_copy(symbol_table_t *src)
{
    symbol_table_t *new_table = symbol_table_init();
    cm_hash_table_free(new_table->store);
    new_table->store = cm_hash_table_copy(src->store, _strdup, _copy_symbol);
    new_table->nentries = src->nentries;
    return new_table;
}

compiler_t *
compiler_init_with_state(symbol_table_t *symbol_table, cm_array_list *constants)
{
    compiler_t *compiler = compiler_init();
    free_symbol_table(compiler->symbol_table);
    compiler->symbol_table = symbol_table_copy(symbol_table);
    compiler->constants_pool = cm_array_list_copy(constants, _copy_monkey_object);
    return compiler;
}

void
compiler_free(compiler_t *compiler)
{
    cm_array_list_free(compiler->scopes);
    if (compiler->constants_pool)
        cm_array_list_free(compiler->constants_pool);
    free_symbol_table(compiler->symbol_table);
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

static int
compare_monkey_hash_keys(const void *v1, const void *v2)
{
    node_t *n1 = (node_t *) v1;
    node_t *n2 = (node_t *) v2;
    char *s1 = n1->string(n1);
    char *s2 = n2->string(n2);
    int ret = strcmp(s1, s2);
    free(s1);
    free(s2);
    return ret;
}

static void
replace_last_pop_with_return(compiler_t *compiler)
{
    compilation_scope_t *top_scope = get_top_scope(compiler);
    size_t lastpos = top_scope->last_instruction.position;
    instructions_t *new_ins = instruction_init(OPRETURNVALUE);
    replace_instruction(compiler, lastpos, new_ins);
    instructions_free(new_ins);
    top_scope->last_instruction.opcode = OPRETURNVALUE;
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
    identifier_t *ident_exp;
    if_expression_t *if_exp;
    monkey_int_t *int_obj;
    monkey_bool_t *bool_obj;
    string_t *str_exp;
    monkey_string_t *str_obj;
    array_literal_t *array_exp;
    hash_literal_t *hash_exp;
    index_expression_t *index_exp;
    function_literal_t *func_exp;
    size_t constant_idx;
    size_t opjmpfalse_pos, after_consequence_pos, jmp_pos, after_alternative_pos;
    compilation_scope_t *scope;
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
        constant_idx = add_constant(compiler, (monkey_object_t *) int_obj);
        emit(compiler, OPCONSTANT, constant_idx);
        break;
    case BOOLEAN_EXPRESSION:
        bool_exp = (boolean_expression_t *) expression_node;
        bool_obj = create_monkey_bool(bool_exp->value);
        if (bool_obj->value)
            emit(compiler, OPTRUE);
        else
            emit(compiler, OPFALSE);
        break;
    case STRING_EXPRESSION:
        str_exp = (string_t *) expression_node;
        str_obj = create_monkey_string(str_exp->value, strlen(str_exp->value));
        size_t constant_idx = add_constant(compiler, (monkey_object_t *) str_obj);
        emit(compiler, OPCONSTANT, constant_idx);
        break;
    case IF_EXPRESSION:
        if_exp = (if_expression_t *) expression_node;
        error = compile(compiler, (node_t *) if_exp->condition);
        if (error.code != COMPILER_ERROR_NONE)
            return error;
        opjmpfalse_pos = emit(compiler, OPJMPFALSE, 9999);
        error = compile(compiler, (node_t *) if_exp->consequence);
        if (error.code != COMPILER_ERROR_NONE)
            return error;
        if (last_instruction_is(compiler, OPPOP))
            remove_last_instruction(compiler);
        jmp_pos = emit(compiler, OPJMP, 9999);
        scope = get_top_scope(compiler);
        after_consequence_pos = scope->instructions->length;
        change_operand(compiler, opjmpfalse_pos, after_consequence_pos);
        if (if_exp->alternative == NULL) {
            emit(compiler, OPNULL);
        } else {
            error = compile(compiler, (node_t *) if_exp->alternative);
            if (error.code != COMPILER_ERROR_NONE)
                return error;
            if (last_instruction_is(compiler, OPPOP))
                remove_last_instruction(compiler);
        }
        after_alternative_pos = scope->instructions->length;
        change_operand(compiler, jmp_pos, after_alternative_pos);
        break;
    case IDENTIFIER_EXPRESSION:
        ident_exp = (identifier_t *) expression_node;
        symbol_t *sym = symbol_resolve(compiler->symbol_table, ident_exp->value);
        if (sym == NULL) {
            error.code = COMPILER_UNDEFINED_VARIABLE;
            error.msg = get_err_msg("undefined variable: %s\n", ident_exp->value);
            return error;
        }
        emit(compiler, OPGETGLOBAL, sym->index);
        break;
    case ARRAY_LITERAL:
        array_exp = (array_literal_t *) expression_node;
        for (size_t i = 0; i < array_exp->elements->length; i++) {
            error = compile(compiler, cm_array_list_get(array_exp->elements, i));
            if (error.code != COMPILER_ERROR_NONE)
                return error;
        }
        emit(compiler, OPARRAY, array_exp->elements->length);
        break;
    case HASH_LITERAL:
        hash_exp = (hash_literal_t *) expression_node;
        cm_array_list *keys = cm_hash_table_get_keys(hash_exp->pairs);
        if (keys != NULL) {
           cm_array_list_sort(keys, sizeof(node_t *), compare_monkey_hash_keys);
            for (size_t i = 0; i < keys->length; i++) {
                node_t *key = (node_t *) cm_array_list_get(keys, i);
                node_t *value = (node_t *) cm_hash_table_get(hash_exp->pairs, key);
                error = compile(compiler, key);
                if (error.code != COMPILER_ERROR_NONE)
                    return error;
                error = compile(compiler, value);
                if (error.code != COMPILER_ERROR_NONE)
                    return error;
            }
            cm_array_list_free(keys);
        }
        emit(compiler, OPHASH, 2 * hash_exp->pairs->nkeys);
        break;
    case INDEX_EXPRESSION:
        index_exp = (index_expression_t *) expression_node;
        error = compile(compiler, (node_t *) index_exp->left);
        if (error.code != COMPILER_ERROR_NONE)
            return error;
        error = compile(compiler, (node_t *) index_exp->index);
        if (error.code != COMPILER_ERROR_NONE)
            return error;
        emit(compiler, OPINDEX);
        break;
    case FUNCTION_LITERAL:
        func_exp = (function_literal_t *) expression_node;
        compiler_enter_scope(compiler);
        error = compile(compiler, (node_t *) func_exp->body);
        if (error.code != COMPILER_ERROR_NONE)
            return error;
        if (last_instruction_is(compiler, OPPOP))
            replace_last_pop_with_return(compiler);
        if (!last_instruction_is(compiler, OPRETURNVALUE))
            emit(compiler, OPRETURN);
        instructions_t *ins = compiler_leave_scope(compiler);
        monkey_compiled_fn_t *compiled_fn = create_monkey_compiled_fn(ins);
        constant_idx = add_constant(compiler, (monkey_object_t *) compiled_fn);
        emit(compiler, OPCONSTANT, constant_idx);
        break;
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
    block_statement_t *block_stmt;
    letstatement_t *let_stmt;
    return_statement_t *ret_stmt;
    size_t i;
    switch (statement_node->statement_type) {
    case EXPRESSION_STATEMENT:
        expression_stmt = (expression_statement_t *) statement_node;
        error = compile_expression_node(compiler, expression_stmt->expression);
        if (error.code != COMPILER_ERROR_NONE)
            return error;
        emit(compiler, OPPOP);
        break;
    case BLOCK_STATEMENT:
        block_stmt = (block_statement_t *) statement_node;
        for (i = 0; i < block_stmt->nstatements; i++) {
            error = compile(compiler, (node_t *) block_stmt->statements[i]);
            if (error.code != COMPILER_ERROR_NONE)
                return error;
        }
        break;
    case LET_STATEMENT:
        let_stmt = (letstatement_t *) statement_node;
        error = compile(compiler, (node_t *) let_stmt->value);
        if (error.code != COMPILER_ERROR_NONE)
            return error;
        symbol_t *sym = symbol_define(compiler->symbol_table, let_stmt->name->value);
        emit(compiler, OPSETGLOBAL, sym->index);
        break;
    case RETURN_STATEMENT:
        ret_stmt = (return_statement_t *) statement_node;
        error = compile(compiler, (node_t *) ret_stmt->return_value);
        if (error.code != COMPILER_ERROR_NONE)
            return error;
        emit(compiler, OPRETURNVALUE);
        break;
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
    compilation_scope_t *scope = get_top_scope(compiler);
    bytecode->instructions = scope->instructions;
    bytecode->constants_pool = compiler->constants_pool;
    return bytecode;
}

compilation_scope_t *
get_top_scope(compiler_t *compiler)
{
    return (compilation_scope_t *) cm_array_list_get(compiler->scopes, compiler->scope_index);
}


void
scope_free(compilation_scope_t *scope)
{
    instructions_free(scope->instructions);
    free(scope);
}

static instructions_t *
copy_instructions(instructions_t *ins)
{
    instructions_t *ret;
    ret = malloc(sizeof(*ret));
    if (ret == NULL)
        err(EXIT_FAILURE, "malloc failed");
    ret->bytes = malloc(ins->length);
    if (ret->bytes == NULL)
        err(EXIT_FAILURE, "malloc failed");
    for (size_t i = 0; i < ins->length; i++)
        ret->bytes[i] = ins->bytes[i];
    ret->length = ins->length;
    ret->size = ins->size;
    return ret;
}

instructions_t *
compiler_leave_scope(compiler_t *compiler)
{
    compilation_scope_t *scope = get_top_scope(compiler);
    instructions_t *ins = copy_instructions(scope->instructions);
    cm_array_list_remove(compiler->scopes, compiler->scope_index);
    compiler->scope_index--;
    return ins;
}

void
compiler_enter_scope(compiler_t *compiler)
{
    compilation_scope_t *scope = scope_init();
    cm_array_list_add(compiler->scopes, scope);
    compiler->scope_index++;
}