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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "parser_tracing.h"

static expression_t * parse_identifier_expression(parser_t *);
static expression_t * parse_integer_expression(parser_t *);
static expression_t * parse_string_expression(parser_t *);
static expression_t * parse_prefix_expression(parser_t *);
static expression_t * parse_boolean_expression(parser_t *);
static expression_t * parse_grouped_expression(parser_t *);
static expression_t * parse_if_expression(parser_t *);
static expression_t * parse_function_literal(parser_t *);
static expression_t * parse_array_literal(parser_t *);

static expression_t * parse_infix_expression(parser_t *, expression_t *);
static expression_t * parse_call_expression(parser_t *, expression_t *);

 static prefix_parse_fn prefix_fns [] = {
     NULL, //ILLEGAL
     NULL, //END OF FILE
     parse_identifier_expression, //IDENT
     parse_integer_expression, //INT
     parse_string_expression, //STRING
     NULL, //ASSIGN
     NULL, //PLUS
     parse_prefix_expression, //MINUS
     parse_prefix_expression, //BANG
     NULL, //SLASH
     NULL, //ASTERISK
     NULL, //LT
     NULL, //GT
     NULL, //EQ
     NULL, //NOT_EQ
     NULL, //COMMA
     NULL, //SEMICOLON
     parse_grouped_expression, //LPAREN
     NULL, //RPAREN
     NULL, //LBRACE
     NULL, //RBRACE
     parse_array_literal, //LBRACKET
     NULL, //RBRACKET
     parse_function_literal, //FUNCTION
     NULL, //LET
     parse_if_expression, //IF
     NULL, //ELSE
     NULL, //RETURN
     parse_boolean_expression, //TRUE
     parse_boolean_expression, //FALSE
 };

 static infix_parse_fn infix_fns [] = {
     NULL, //ILLEGAL
     NULL, //END OF FILE
     NULL, //IDENT
     NULL, //INT
     NULL, //STRING
     NULL, //ASSIGN
     parse_infix_expression, //PLUS
     parse_infix_expression, //MINUS
     NULL, //BANG
     parse_infix_expression, //SLASH
     parse_infix_expression, //ASTERISK
     parse_infix_expression, //LT
     parse_infix_expression, //GT
     parse_infix_expression, //EQ
     parse_infix_expression, //NOT_EQ
     NULL, //COMMA
     NULL, //SEMICOLON
     parse_call_expression, //LPAREN
     NULL, //RPAREN
     NULL, //LBRACE
     NULL, //RBRACE
     NULL, //FUNCTION
     NULL, //LET
     NULL, //IF
     NULL, //ELSE
     NULL, //RETURN
     NULL, //TRUE
     NULL, //FALSE
 };


static void
add_parse_error(parser_t *parser, char *errmsg)
{
    if (parser->errors == NULL) {
        parser->errors = cm_list_init();
        if (parser->errors == NULL)
            errx(EXIT_FAILURE, "malloc failed");
    }
    cm_list_add(parser->errors, errmsg);
}

static void
handle_no_prefix_fn(parser_t *parser)
{
    char *msg = NULL;
    asprintf(&msg, "no prefix parse function for the token \"%s\"", parser->cur_tok->literal);
    if (msg == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    add_parse_error(parser, msg);
}

static operator_precedence_t
precedence(token_type tok_type)
{
    switch (tok_type) {
        case EQ:
        case NOT_EQ:
           return EQUALS;
        case LT:
        case GT:
            return LESSGREATER;
        case PLUS:
        case MINUS:
            return SUM;
        case SLASH:
        case ASTERISK:
            return PRODUCT;
        case LPAREN:
            return CALL;
        default:
            return LOWEST;
    }
}

 static operator_precedence_t
 peek_precedence(parser_t *parser)
 {
     return precedence(parser->peek_tok->type);
 }

 static operator_precedence_t
 cur_precedence(parser_t *parser)
 {
     return precedence(parser->cur_tok->type);
 }

static char *
program_token_literal(void *prog_obj)
{
    program_t *program = (program_t *) prog_obj;
    if (program == NULL)
        return "";
    
    node_t head_node = program->statements[0]->node;
    return head_node.token_literal(&head_node);
}

static char *
letstatement_token_literal(void *stmt)
{
    letstatement_t *ls = (letstatement_t *) stmt;
    return ls->token->literal;
}

static char *
return_statement_token_literal(void *stmt)
{
    return_statement_t *ret_stmt = (return_statement_t *) stmt;
    return ret_stmt->token->literal;
}

static char *
string_token_literal(void *exp)
{
    string_t *string = (string_t *) exp;
    return string->token->literal;
}

static char *
expression_statement_token_literal(void *stmt)
{
    expression_statement_t *exp_stmt = (expression_statement_t *) stmt;
    return exp_stmt->token->literal;
}

static char *
array_literal_token_literal(void *exp)
{
    array_literal_t *array = (array_literal_t *) exp;
    return array->token->literal;
}

static char *
block_statement_token_literal(void *stmt)
{
    block_statement_t *block_stmt = (block_statement_t *) stmt;
    return block_stmt->token->literal;
}

static char *
prefix_expression_token_literal(void *exp)
{
    prefix_expression_t *prefix_exp = (prefix_expression_t *) exp;
    return prefix_exp->token->literal;
}

static char *
infix_expression_token_literal(void *exp)
{
    infix_expression_t *infix_exp = (infix_expression_t *) exp;
    return infix_exp->token->literal;
}

static char *
boolean_expression_token_literal(void *exp)
{
    boolean_expression_t *bool_exp = (boolean_expression_t *) exp;
    return bool_exp->token->literal;
}

static char *
if_expression_token_literal(void *exp)
{
    if_expression_t *if_exp = (if_expression_t *) exp;
    return if_exp->token->literal;
}

void
_expression_node(void)
{
}

static char *
string_string(void *exp)
{
    string_t *string = (string_t *) exp;
    return strdup(string->value);
}

static char *
letstatement_string(void *stmt)
{
    letstatement_t *let_stmt = (letstatement_t *) stmt;
    char *let_stmt_string = NULL;
    char *ident_string = let_stmt->name->expression.node.string(let_stmt->name);
    char *value_string = let_stmt->value? let_stmt->value->node.string(let_stmt->value): strdup("");
    char *let_string = strdup(let_stmt->token->literal);//let_stmt->statement.node.string(let_stmt);
    asprintf(&let_stmt_string, "%s %s = %s;", let_string, ident_string, value_string);
    free(ident_string);
    free(value_string);
    free(let_string);
    if (let_stmt_string == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    return let_stmt_string;
}

static char *
return_statement_string(void *stmt)
{
    return_statement_t *ret_stmt = (return_statement_t *) stmt;
    char *ret_stmt_string = NULL;
    char *value_string = ret_stmt->return_value? ret_stmt->return_value->node.string(ret_stmt->return_value): strdup("");
    asprintf(&ret_stmt_string, "%s %s;", ret_stmt->statement.node.string(ret_stmt), value_string);
    if (ret_stmt_string == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    return ret_stmt_string;
}

static char *
identifier_string(void *id)
{
    identifier_t *ident = (identifier_t *) id;
    char *ident_string = strdup(ident->value);
    if (ident_string == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    return ident_string;
}

static char *
integer_string(void *node)
{
    integer_t *int_exp = (integer_t *) node;
    return long_to_string(int_exp->value);
}

static char *
prefix_expression_string(void *node)
{
    prefix_expression_t *prefix_exp = (prefix_expression_t *) node;
    char *str = NULL;
    char *operand_string = prefix_exp->right->node.string(prefix_exp->right);
    asprintf(&str, "(%s%s)", prefix_exp->operator, operand_string);
    free(operand_string);
    if (str == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    return str;
}

static char *
infix_expression_string(void *node)
{
    infix_expression_t *infix_exp = (infix_expression_t *) node;
    char *str = NULL;
    char *left_string = infix_exp->left->node.string(infix_exp->left);
    char *right_string = infix_exp->right->node.string(infix_exp->right);
    asprintf(&str, "(%s %s %s)", left_string, infix_exp->operator, right_string);
    free(left_string);
    free(right_string);
    if (str == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    return str;
}

static char *
expression_statement_string(void *stmt)
{
    expression_statement_t *exp_stmt = (expression_statement_t *) stmt;
    if (exp_stmt->expression) {
        return (exp_stmt->expression->node.string(exp_stmt->expression));
    }
    return strdup("");
}

static char *
block_statement_string(void *stmt)
{
    block_statement_t *block_stmt = (block_statement_t *) stmt;
    char *string = NULL;
    for (size_t i = 0; i < block_stmt->nstatements; i++) {
        statement_t *s = block_stmt->statements[i];
        char *stmt_string = s->node.string(s);
        if (string == NULL)
            string = stmt_string;
        else {
            char *temp = NULL;
            asprintf(&temp, "%s %s", string, stmt_string);
            if (temp == NULL) {
                free(stmt_string);
                free(string);
                errx(EXIT_FAILURE, "malloc failed");
            }
            free(string);
            free(stmt_string);
            string = temp;
        }
    }
    return string;
}

static char *
boolean_expression_string(void *exp)
{
    //TODO: error check for strdup needed
    boolean_expression_t *bool_exp = (boolean_expression_t *) exp;
    if (bool_exp->value)
        return strdup("true");
    else
        return strdup("false");
}

static char *
if_expression_string(void *exp)
{
    if_expression_t *if_exp = (if_expression_t *) exp;
    char *string = NULL;
    char *condition_string = if_exp->condition->node.string(if_exp->condition);
    char *consequence_string = if_exp->consequence->statement.node.string(if_exp->consequence);
    if (if_exp->alternative != NULL) {
        char *alternative_string = if_exp->alternative->statement.node.string(if_exp->alternative);
        asprintf(&string, "if%s %s else %s", condition_string, consequence_string, alternative_string);
        free(alternative_string);
    } else {
        asprintf(&string, "if%s %s", condition_string, consequence_string);
    }

    free(condition_string);
    free(consequence_string);

    if (string == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    return string;
}

static char *
array_literal_string(void *exp)
{
    array_literal_t *array = (array_literal_t *) exp;
    char *string = NULL;
    char *temp = NULL;
    int ret;
    for (size_t i = 0; i < array->elements->length; i++) {
        expression_t *element = (expression_t *) cm_array_list_get(array->elements, i);
        char *element_string = element->node.string(element);
        if (string == NULL)
            ret = asprintf(&temp, "%s", element_string);
        else {
            ret = asprintf(&temp, "%s, %s", string, element_string);
            free(string);
        }
        free(element_string);
        if (ret == -1)
            errx(EXIT_FAILURE, "malloc failed");
        string = temp;
        temp = NULL;
    }
    ret = asprintf(&temp, "[%s]", string);
    if (ret == -1)
        errx(EXIT_FAILURE, "malloc failed");
    free(string);
    return temp;
}

char *
join_parameters_list(cm_list *parameters_list)
{
    char *string = NULL;
    char *temp = NULL;
    if (parameters_list == NULL || parameters_list->length == 0)
        return strdup("");

    cm_list_node *list_node = parameters_list->head;
    while (list_node != NULL) {
        identifier_t *param = (identifier_t *) list_node->data;
        char *param_string = param->expression.node.string(param);
        if (string == NULL) {
            asprintf(&temp, "%s", param_string);
            free(param_string);
            if (temp == NULL)
                errx(EXIT_FAILURE, "malloc failed");
            string = temp;
        } else {
            asprintf(&temp, "%s, %s", string, param_string);
            free(param_string);
            free(string);
            if (temp == NULL)
                errx(EXIT_FAILURE, "malloc failed");
            string = temp;
        }
        list_node = list_node->next;
    }
    return string;
}

static char *
function_literal_string(void *exp)
{
    function_literal_t *func = (function_literal_t *) exp;
    char *params_string = join_parameters_list(func->parameters);
    char *func_string = NULL;
    char *func_token_literal = func->expression.node.token_literal(func);
    char *body_string = func->body->statement.node.string(func->body);
    asprintf(&func_string, "%s(%s) %s", func_token_literal, params_string, body_string);
    free(params_string);
    free(body_string);
    if (func_string == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    return func_string;
}

static char *
function_literal_token_literal(void *exp)
{
    function_literal_t *func = (function_literal_t *) exp;
    return func->token->literal;
}

static char *
call_expression_string(void *exp)
{
    call_expression_t *call_exp = (call_expression_t *) exp;
    char *args_string = join_parameters_list(call_exp->arguments);
    char *function_string = call_exp->function->node.string(call_exp->function);
    char *string = NULL;
    asprintf(&string, "%s(%s)", function_string, args_string);
    free(function_string);
    free(args_string);
    if (string == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    return string;
}

static char *
call_expression_token_literal(void *exp)
{
    call_expression_t *call_exp = (call_expression_t *) exp;
    return call_exp->token->literal;
}

static char *
program_string(void *prog_ptr)
{
    //TODO: maybe we could optimize this
    program_t *program = (program_t *) prog_ptr;
    char *prog_string = NULL;
    char *temp_string = NULL;
    for (int i = 0; i < program->nstatements; i++) {
        statement_t *stmt = program->statements[i];
        char *stmt_string = stmt->node.string(stmt);
        if (prog_string != NULL) {
            asprintf(&temp_string, "%s %s", prog_string, stmt_string);
            free(prog_string);
        } else
            asprintf(&temp_string, "%s", stmt_string);
        free(stmt_string);
        if (temp_string == NULL) {
            if (prog_string != NULL)
                free(prog_string);
            errx(EXIT_FAILURE, "malloc failed");
        }
        prog_string = temp_string;
        temp_string = NULL;
    }
    return prog_string;
}

static letstatement_t *
create_letstatement(parser_t *parser)
{
    letstatement_t *let_stmt;
    let_stmt = malloc(sizeof(*let_stmt));
    if (let_stmt == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    let_stmt->token = token_copy(parser->cur_tok);
    if (let_stmt->token == NULL) {
        free(let_stmt);
        errx(EXIT_FAILURE, "malloc failed");
    }
    let_stmt->statement.statement_type = LET_STATEMENT;
    let_stmt->statement.node.token_literal = letstatement_token_literal;
    let_stmt->statement.node.string = letstatement_string;
    let_stmt->statement.node.type = STATEMENT;
    let_stmt->name = NULL;
    let_stmt->value = NULL;
    return let_stmt;
}

static return_statement_t *
create_return_statement(parser_t *parser)
{
    return_statement_t *ret_stmt;
    ret_stmt = malloc(sizeof(*ret_stmt));
    if (ret_stmt == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    ret_stmt->token = token_copy(parser->cur_tok);
    if (ret_stmt->token == NULL) {
        free(ret_stmt);
        errx(EXIT_FAILURE, "malloc failed");
    }
    ret_stmt->return_value = NULL;
    ret_stmt->statement.statement_type = RETURN_STATEMENT;
    ret_stmt->statement.node.token_literal = return_statement_token_literal;
    ret_stmt->statement.node.string = return_statement_string;
    ret_stmt->statement.node.type = STATEMENT;
    return ret_stmt;
}

static expression_statement_t *
create_expression_statement(parser_t *parser)
{
    expression_statement_t *exp_stmt;
    exp_stmt = malloc(sizeof(*exp_stmt));
    if (exp_stmt == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    exp_stmt->token = token_copy(parser->cur_tok);
    if (exp_stmt->token == NULL) {
        free(exp_stmt);
        errx(EXIT_FAILURE, "malloc failed");
    }
    exp_stmt->expression = NULL;
    exp_stmt->statement.statement_type = EXPRESSION_STATEMENT;
    exp_stmt->statement.node.token_literal = expression_statement_token_literal;
    exp_stmt->statement.node.string = expression_statement_string;
    exp_stmt->statement.node.type = STATEMENT;
    return exp_stmt;
}

static block_statement_t *
create_block_statement(parser_t *parser)
{
    block_statement_t *block_stmt;
    block_stmt = malloc(sizeof(*block_stmt));
    if (block_stmt == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    block_stmt->statement.node.string = block_statement_string;
    block_stmt->statement.node.token_literal = block_statement_token_literal;
    block_stmt->statement.node.type = STATEMENT;
    block_stmt->statement.statement_type = BLOCK_STATEMENT;
    block_stmt->array_size = 8;
    block_stmt->statements = calloc(block_stmt->array_size, sizeof(*block_stmt->statements));
    if (block_stmt->statements == NULL) {
        free(block_stmt);
        errx(EXIT_FAILURE, "malloc failed");
    }
    block_stmt->nstatements = 0;
    block_stmt->token = token_copy(parser->cur_tok);
    return block_stmt;
}

static function_literal_t *
create_function_literal(parser_t *parser)
{
    function_literal_t *func;
    func = malloc(sizeof(*func));
    if (func == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    func->expression.node.string = function_literal_string;
    func->expression.node.token_literal = function_literal_token_literal;
    func->expression.node.type = EXPRESSION;
    func->expression.expression_type = FUNCTION_LITERAL;
    func->expression.expression_node = NULL;
    func->parameters = cm_list_init();
    func->token = token_copy(parser->cur_tok);
    func->body = NULL;
    return func;
}

static call_expression_t *
create_call_expression(parser_t *parser)
{
    call_expression_t *call_exp;
    call_exp = malloc(sizeof(*call_exp));
    if (call_exp == NULL)
        errx(EXIT_FAILURE, "malloc failed");

    call_exp->expression.node.token_literal = call_expression_token_literal;
    call_exp->expression.node.string = call_expression_string;
    call_exp->expression.node.type = EXPRESSION;
    call_exp->expression.expression_type = CALL_EXPRESSION;
    call_exp->expression.expression_node = NULL;
    call_exp->arguments = cm_list_init();
    if (call_exp->arguments == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    call_exp->token = token_copy(parser->peek_tok);
    if (call_exp->token == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    call_exp->function = NULL;
    return call_exp;
}

void
parser_free(parser_t *parser)
{
    lexer_free(parser->lexer);
    token_free(parser->cur_tok);
    token_free(parser->peek_tok);
    cm_list_free(parser->errors, NULL);
    free(parser);
}

void
program_free(program_t *program)
{
    if (program == NULL)
        return;
    
    for (int i = 0; i < program->nstatements; i++) {
        statement_t *stmt = program->statements[i];
        if (stmt)
            free_statement(stmt);
    }
    free(program->statements);
    free(program);
}

void *
create_statement(parser_t *parser, statement_type_t stmt_type)
{
    switch (stmt_type) {
        case LET_STATEMENT:
            return create_letstatement(parser);
        case RETURN_STATEMENT:
            return create_return_statement(parser);
        case EXPRESSION_STATEMENT:
            return create_expression_statement(parser);
        case BLOCK_STATEMENT:
            return create_block_statement(parser);
        default:
            return NULL;
    }
}

static void
free_identifier(void *id)
{
    identifier_t *ident = (identifier_t *) id;
    token_free(ident->token);
    free(ident->value);
    free(ident);
}

static void
free_integer_expression(integer_t *int_exp)
{
    token_free(int_exp->token);
    free(int_exp);
}

static void
free_prefix_expression(prefix_expression_t *prefix_exp)
{
    token_free(prefix_exp->token);
    free(prefix_exp->operator);
    free_expression(prefix_exp->right);
    free(prefix_exp);
}

static void
free_infix_expression(infix_expression_t *infix_exp)
{
    token_free(infix_exp->token);
    free(infix_exp->operator);
    free_expression(infix_exp->left);
    free_expression(infix_exp->right);
    free(infix_exp);
}

static void
free_boolean_expression(boolean_expression_t *bool_exp)
{
    token_free(bool_exp->token);
    free(bool_exp);
}

static void
free_string(string_t *string)
{
    free(string->value);
    token_free(string->token);
    free(string);
}

static void
free_return_statement(return_statement_t *ret_stmt)
{
    if (ret_stmt->token)
        token_free(ret_stmt->token);
    if (ret_stmt->return_value)
        free_expression(ret_stmt->return_value);
    free(ret_stmt);
}

static void
free_letstatement(letstatement_t *let_stmt)
{
    //TODO: free other things also
    if (let_stmt->name)
        free_identifier(let_stmt->name);
    if (let_stmt->token)
        token_free(let_stmt->token);
    if (let_stmt->value)
        free_expression(let_stmt->value);
    free(let_stmt);
}

static void
free_if_expression(if_expression_t *if_exp)
{
    token_free(if_exp->token);
    if (if_exp->condition)
        free_expression(if_exp->condition);
    if (if_exp->consequence)
        free_statement((statement_t *) if_exp->consequence);
    if (if_exp->alternative)
        free_statement((statement_t *) if_exp->alternative);
    free(if_exp);
}

static void
free_function_literal(function_literal_t *function)
{
    if (function->body)
        free_statement((statement_t *) function->body);
    if (function->parameters)
        cm_list_free(function->parameters, free_identifier);
    token_free(function->token);
    free(function);
}

static void
free_call_expression(call_expression_t *call_exp)
{
    if (call_exp->function)
        free_expression(call_exp->function);
    cm_list_free(call_exp->arguments, free_expression);
    token_free(call_exp->token);
    free(call_exp);
}

static void
free_array_literal(array_literal_t *array)
{
    if (array->elements->free_func) {
        cm_array_list_free(array->elements);
    }
    token_free(array->token);
    free(array);
}

void
free_expression(void *e)
{
    expression_t *exp = (expression_t *) e;
    switch (exp->expression_type)
    {
        case IDENTIFIER_EXPRESSION:
            free_identifier((identifier_t *) exp);
            break;
        case INTEGER_EXPRESSION:
            free_integer_expression((integer_t *) exp);
            break;
        case PREFIX_EXPRESSION:
            free_prefix_expression((prefix_expression_t *) exp);
            break;
        case INFIX_EXPRESSION:
            free_infix_expression((infix_expression_t *) exp);
            break;
        case BOOLEAN_EXPRESSION:
            free_boolean_expression((boolean_expression_t *) exp);
            break;
        case IF_EXPRESSION:
            free_if_expression((if_expression_t *) exp);
            break;
        case FUNCTION_LITERAL:
            free_function_literal((function_literal_t *) exp);
            break;
        case CALL_EXPRESSION:
            free_call_expression((call_expression_t *) exp);
            break;
        case STRING_EXPRESSION:
            free_string((string_t *) exp);
            break;
        case ARRAY_LITERAL:
            free_array_literal((array_literal_t *) exp);
            break;
        default:
            break;
    }
}

static void
free_expression_statement(expression_statement_t *exp_stmt)
{
    if (exp_stmt->token)
        token_free(exp_stmt->token);
    if (exp_stmt->expression)
        free_expression(exp_stmt->expression);
    free(exp_stmt);
}

static void
free_block_statement(block_statement_t *block_stmt)
{
    for (size_t i = 0; i < block_stmt->nstatements; i++) {
        free_statement(block_stmt->statements[i]);
    }
    free(block_stmt->statements);
    token_free(block_stmt->token);
    free(block_stmt);
}

void
free_statement(statement_t *stmt)
{

    switch (stmt->statement_type)
    {
        case LET_STATEMENT:
            free_letstatement((letstatement_t *) stmt);
            break;
        case RETURN_STATEMENT:
            free_return_statement((return_statement_t *) stmt);
            break;
        case EXPRESSION_STATEMENT:
            free_expression_statement((expression_statement_t *) stmt);
            break;
        case BLOCK_STATEMENT:
            free_block_statement((block_statement_t *) stmt);
            break;
        default:
            free(stmt);
            break;
    }
}

parser_t *
parser_init(lexer_t *l)
{
    parser_t *parser;
    parser = malloc(sizeof(*parser));
    if (parser == NULL)
        return NULL;
    parser->lexer = l;
    parser->cur_tok = NULL;
    parser->peek_tok = NULL;
    parser->errors = NULL;
    parser_next_token(parser);
    parser_next_token(parser);
    return parser;
}

void
parser_next_token(parser_t *parser)
{
    if (parser->cur_tok)
        token_free(parser->cur_tok);
    parser->cur_tok = parser->peek_tok;
    parser->peek_tok = lexer_next_token(parser->lexer);
}

static int
add_statement_to_program(program_t *program, statement_t *stmt)
{
    if (program->nstatements == program->array_size) {
        size_t new_size = program->array_size * 2;
        program->statements = reallocarray(program->statements,
            new_size, sizeof(*program->statements));
        if (program->statements == NULL)
            return 1;
        program->array_size = new_size;
    }
    program->statements[program->nstatements++] = stmt;
    return 0;
}

static int
add_statement_to_block(block_statement_t *block_stmt, statement_t *stmt)
{
    if (block_stmt->nstatements == block_stmt->array_size) {
        size_t new_size = block_stmt->array_size * 2;
        block_stmt->statements = reallocarray(block_stmt->statements,
            new_size, sizeof(*block_stmt->statements));
        if (block_stmt->statements == NULL)
            return 1;
        block_stmt->array_size = new_size;
    }
    block_stmt->statements[block_stmt->nstatements++] = stmt;
    return 0;
}


static void
peek_error(parser_t *parser, token_type tok_type)
{
    char *msg = NULL;
    asprintf(&msg, "expected next token to be %s, got %s instead",
        get_token_name_from_type(tok_type),
        get_token_name_from_type(parser->peek_tok->type));
    if (msg == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    add_parse_error(parser, msg);
}

static int
expect_peek(parser_t *parser, token_type tok_type)
{
    if (parser->peek_tok->type == tok_type) {
        parser_next_token(parser);
        return 1;
    }
    peek_error(parser, tok_type);
    return 0;
}

static char *
ident_token_literal(void *node)
{
    identifier_t *ident = (identifier_t *) node;
    return ident->token->literal;
}

static identifier_t *
create_identifier(parser_t *parser)
{
    identifier_t *ident;
    ident = malloc(sizeof(*ident));
    if (ident == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    ident->token = token_copy(parser->cur_tok);
    if (ident->token == NULL) {
        free(ident);
        errx(EXIT_FAILURE, "malloc failed");
    }
    ident->expression.node.token_literal = ident_token_literal;
    ident->expression.expression_type = IDENTIFIER_EXPRESSION;
    ident->expression.node.string = identifier_string;
    ident->expression.node.type = EXPRESSION;
    ident->value = strdup(parser->cur_tok->literal);
    if (ident->value == NULL) {
        token_free(ident->token);
        free(ident);
        errx(EXIT_FAILURE, "malloc failed");
    }
    return ident;
}

//TODO: can we create a create_expression API like we have for statements?
static expression_t *
parse_identifier_expression(parser_t * parser)
{
    identifier_t *ident = create_identifier(parser);
    return (expression_t *) ident;
}

static expression_t *
parse_expression(parser_t * parser, operator_precedence_t precedence)
{
    #ifdef TRACE
        trace("parse_expression");
    #endif
    prefix_parse_fn prefix_fn = prefix_fns[parser->cur_tok->type];
    if (prefix_fn == NULL) {
        handle_no_prefix_fn(parser);
        return NULL;
    }
    expression_t *left_exp = prefix_fn(parser);

    for (;;) {
        if (parser->peek_tok->type == SEMICOLON)
            break;
        if (precedence >= peek_precedence(parser))
            break;

        infix_parse_fn infix_fn = infix_fns[parser->peek_tok->type];
        if (infix_fn == NULL)
            return left_exp;

        parser_next_token(parser);
        expression_t *right = infix_fn(parser, left_exp);
        left_exp = right;
    }

    #ifdef TRACE
        untrace("parse_expression");
    #endif
    return left_exp;
}

static letstatement_t *
parse_letstatement(parser_t *parser)
{
    letstatement_t *let_stmt = (letstatement_t *) create_statement(parser, LET_STATEMENT);
    if (let_stmt == NULL)
        errx(EXIT_FAILURE, "malloc failed"); // returning NULL would indicate no valid token
    
    if (!expect_peek(parser, IDENT)) {
        free_statement((statement_t *) let_stmt);
        return NULL;
    }

    identifier_t *ident = (identifier_t *) parse_identifier_expression(parser);
    let_stmt->name = ident;
    if (!expect_peek(parser, ASSIGN)) {
        free_statement((statement_t *) let_stmt);
        return NULL;
    }
    parser_next_token(parser);
    let_stmt->value = parse_expression(parser, LOWEST);
    if (parser->peek_tok->type == SEMICOLON)
        parser_next_token(parser);

    return let_stmt;
}

static return_statement_t *
parse_return_statement(parser_t *parser)
{
    return_statement_t *ret_stmt = (return_statement_t *)
        create_statement(parser, RETURN_STATEMENT);
    parser_next_token(parser);
    ret_stmt->return_value = parse_expression(parser, LOWEST);
    if (parser->peek_tok->type == SEMICOLON)
        parser_next_token(parser);
    return ret_stmt;
}

program_t *
program_init(void)
{
    program_t *program;
    program = malloc(sizeof(*program));
    if (program == NULL)
        return NULL;
    program->node.token_literal = program_token_literal;
    program->node.string = program_string;
    program->node.type = PROGRAM;
    program->array_size = 64;
    program->statements = calloc(64, sizeof(*program->statements));
    if (program->statements == NULL) {
        free(program);
        return NULL;
    }
    program->nstatements = 0;
    return program;
}

program_t *
parse_program(parser_t *parser)
{
    
    program_t *program = program_init();
    if (program == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    while (parser->cur_tok->type != END_OF_FILE) {
        statement_t *stmt = parser_parse_statement(parser);
        if (stmt != NULL) {
            int status = add_statement_to_program(program, stmt);
            if (status != 0) {
                program_free(program);
                return NULL;
            }
        }
        parser_next_token(parser);
    }
    return program;
}

static expression_statement_t *
parse_expression_statement(parser_t *parser)
{
    #ifdef TRACE
        trace("parse_expression_statement");
    #endif
    expression_statement_t *exp_stmt = create_expression_statement(parser);
    exp_stmt->expression = parse_expression(parser, LOWEST);
    if (parser->peek_tok->type == SEMICOLON)
        parser_next_token(parser);
    #ifdef TRACE
        untrace("parse_expression_statement");
    #endif
    return exp_stmt;
}


statement_t *
parser_parse_statement(parser_t *parser)
{
    statement_t *stmt;
    switch(parser->cur_tok->type) {
        case LET:
            stmt = (statement_t *) parse_letstatement(parser);
            return stmt;
        case RETURN:
            stmt = (statement_t *) parse_return_statement(parser);
            return stmt;
        default:
            return (statement_t *) parse_expression_statement(parser);
    }
}

static char *
int_exp_token_literal(void *node)
{
    integer_t *int_exp = (integer_t *) node;
    return int_exp->token->literal;
}

expression_t *
parse_integer_expression(parser_t *parser)
{
    #ifdef TRACE
        trace("parse_integer_expression");
    #endif
    integer_t *int_exp;
    int_exp = malloc(sizeof(*int_exp));
    if (int_exp == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    int_exp->expression.node.token_literal = int_exp_token_literal;
    int_exp->expression.node.string = integer_string;
    int_exp->expression.node.type = EXPRESSION;
    int_exp->expression.expression_type = INTEGER_EXPRESSION;
    int_exp->expression.expression_node = NULL;
    int_exp->token = token_copy(parser->cur_tok);
    errno = 0;
    char *ep;
    int_exp->value = strtol(parser->cur_tok->literal, &ep, 10);
    if (ep == parser->cur_tok->literal || *ep != 0 || errno != 0) {
        char *errmsg = NULL;
        asprintf(&errmsg, "could not parse %s as integer", parser->cur_tok->literal);
        if (errmsg == NULL)
            errx(EXIT_FAILURE, "malloc failed");
        add_parse_error(parser, errmsg);
    }

    #ifdef TRACE
        untrace("parse_integer_expression");
    #endif

    return (expression_t *) int_exp;
}

expression_t *
parse_string_expression(parser_t *parser)
{
    #ifdef TRACE
        trace("parse_string_expression");
    #endif
    string_t *string;
    string = malloc(sizeof(*string));
    if (string == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    string->expression.node.string = string_string;
    string->expression.node.token_literal = string_token_literal;
    string->expression.node.type = EXPRESSION;
    string->expression.expression_type = STRING_EXPRESSION;
    string->expression.expression_node = NULL;
    string->token = token_copy(parser->cur_tok);
    string->value = strdup(parser->cur_tok->literal);
    string->length = strlen(parser->cur_tok->literal);
    if (string->value == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    #ifdef TRACE
        untrace("parse_string_expression");
    #endif
    return (expression_t *) string;
}

expression_t *
parse_prefix_expression(parser_t *parser)
{
    #ifdef TRACE
        trace("parse_prefix_expression");
    #endif
    prefix_expression_t *prefix_exp;
    prefix_exp = malloc(sizeof(*prefix_exp));
    if (prefix_exp == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    prefix_exp->expression.expression_type = PREFIX_EXPRESSION;
    prefix_exp->expression.expression_node = NULL;
    prefix_exp->expression.node.string = prefix_expression_string;
    prefix_exp->expression.node.token_literal = prefix_expression_token_literal;
    prefix_exp->expression.node.type = EXPRESSION;
    prefix_exp->token = token_copy(parser->cur_tok);
    prefix_exp->operator = strdup(parser->cur_tok->literal);
    if (prefix_exp->operator == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    parser_next_token(parser);
    prefix_exp->right = parse_expression(parser, PREFIX);

    #ifdef TRACE
        untrace("parse_prefix_expression");
    #endif
    return (expression_t *) prefix_exp;
}

static expression_t *
parse_infix_expression(parser_t *parser, expression_t *left)
{
    #ifdef TRACE
        trace("parse_infix_expression");
    #endif
    infix_expression_t *infix_exp;
    infix_exp = malloc(sizeof(*infix_exp));
    if (infix_exp == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    infix_exp->expression.expression_node = NULL;
    infix_exp->expression.expression_type = INFIX_EXPRESSION;
    infix_exp->expression.node.string = infix_expression_string;
    infix_exp->expression.node.token_literal = infix_expression_token_literal;
    infix_exp->expression.node.type = EXPRESSION;
    infix_exp->left = left;
    infix_exp->operator = strdup(parser->cur_tok->literal);
    infix_exp->token = token_copy(parser->cur_tok);
    operator_precedence_t precedence = cur_precedence(parser);
    parser_next_token(parser);
    infix_exp->right = parse_expression(parser, precedence);

    #ifdef TRACE
        untrace("parse_infix_expression");
    #endif
    return (expression_t *) infix_exp;
}

static expression_t *
parse_boolean_expression(parser_t *parser)
{
    #ifdef TRACE
        trace("parse_boolean_expression");
    #endif
    boolean_expression_t *bool_exp;
    bool_exp = malloc(sizeof(*bool_exp));
    if (bool_exp == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    bool_exp->token = token_copy(parser->cur_tok);
    bool_exp->expression.expression_node = NULL;
    bool_exp->expression.expression_type = BOOLEAN_EXPRESSION;
    bool_exp->expression.node.token_literal = boolean_expression_token_literal;
    bool_exp->expression.node.string = boolean_expression_string;
    bool_exp->expression.node.type = EXPRESSION;
    if (parser->cur_tok->type == TRUE)
        bool_exp->value = true;
    else
        bool_exp->value = false;

    #ifdef TRACE
        untrace("parse_boolean_expression");
    #endif
    return (expression_t *) bool_exp;
}

static cm_array_list *
parse_expression_list(parser_t *parser, token_type stop_token_type)
{
    cm_array_list *expression_list = cm_array_list_init(4, free_expression);
    if (parser->peek_tok->type == stop_token_type) {
        parser_next_token(parser);
        return expression_list;
    }

    parser_next_token(parser);
    expression_t *exp = parse_expression(parser, LOWEST);
    cm_array_list_add(expression_list, exp);
    while (parser->peek_tok->type == COMMA) {
        parser_next_token(parser);
        parser_next_token(parser);
        exp = parse_expression(parser, LOWEST);
        cm_array_list_add(expression_list, exp);
    }

    if (!expect_peek(parser, stop_token_type)) {
        cm_array_list_free(expression_list);
        return NULL;
    }
    return expression_list;
}

static expression_t *
parse_grouped_expression(parser_t *parser)
{
    #ifdef TRACE
        trace("parse_grouped_expression");
    #endif
    parser_next_token(parser);
    expression_t *exp = parse_expression(parser, LOWEST);
    if (!expect_peek(parser, RPAREN)) {
        free_expression(exp);
        exp = NULL;
    }

    #ifdef TRACE
        untrace("parse_grouped_expression");
    #endif
    return exp;
}

static expression_t *
parse_array_literal(parser_t *parser)
{
    #ifdef TRACE
        trace("parse_array_literal");
    #endif
    array_literal_t *array;
    array = malloc(sizeof(*array));
    if (array == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    array->elements = parse_expression_list(parser, RBRACKET);
    array->token = token_copy(parser->cur_tok);
    array->expression.node.string = array_literal_string;
    array->expression.node.token_literal = array_literal_token_literal;
    array->expression.node.type = EXPRESSION;
    array->expression.expression_type = ARRAY_LITERAL;
    array->expression.expression_node = NULL;
    #ifdef TRACE
        untrace("parse_array_literal");
    #endif
    return (expression_t *) array;
}

static block_statement_t *
parse_block_statement(parser_t *parser)
{
    #ifdef TRACE
        trace("parse_block_statement");
    #endif
    block_statement_t *block_stmt = create_block_statement(parser);
    parser_next_token(parser);
    while (parser->cur_tok->type != RBRACE && parser->cur_tok->type != END_OF_FILE) {
        statement_t *stmt = parser_parse_statement(parser);
        if (stmt != NULL)
            add_statement_to_block(block_stmt, stmt);
        parser_next_token(parser);
    }
    #ifdef TRACE
        untrace("parse_block_statement");
    #endif
    return block_stmt;
}

static expression_t *
parse_if_expression(parser_t *parser)
{
    #ifdef TRACE
        trace("parse_if_expression");
    #endif
    if_expression_t *if_exp;
    if_exp = malloc(sizeof(*if_exp));
    if (if_exp == NULL)
        errx(EXIT_FAILURE, "malloc failed");

    if_exp->expression.node.string = if_expression_string;
    if_exp->expression.node.token_literal = if_expression_token_literal;
    if_exp->expression.node.type = EXPRESSION;
    if_exp->expression.expression_type = IF_EXPRESSION;
    if_exp->token = token_copy(parser->cur_tok);
    if_exp->expression.expression_node = NULL;
    if_exp->condition = NULL;
    if_exp->alternative = NULL;
    if_exp->consequence = NULL;

    if (!expect_peek(parser, LPAREN)) {
        token_free(if_exp->token);
        free(if_exp);
        return NULL;
    }

    parser_next_token(parser);
    if_exp->condition = parse_expression(parser, LOWEST);
    if (!expect_peek(parser, RPAREN)) {
        free_if_expression(if_exp);
        return NULL;
    }

    if (!expect_peek(parser, LBRACE)) {
        free_if_expression(if_exp);
        return NULL;
    }

    if_exp->consequence = parse_block_statement(parser);

    if (parser->peek_tok->type == ELSE) {
        parser_next_token(parser);
        if (!expect_peek(parser, LBRACE)) {
            free_if_expression(if_exp);
            return NULL;
        }
        if_exp->alternative = parse_block_statement(parser);
    }
    #ifdef TRACE
        untrace("parse_if_expression")
    #endif
    return (expression_t *) if_exp;
}

static void
parse_function_parameters(parser_t * parser, function_literal_t *function)
{
    if (parser->peek_tok->type == RPAREN) {
        parser_next_token(parser);
        return;
    }

    parser_next_token(parser);
    identifier_t *identifier = create_identifier(parser);
    cm_list_add(function->parameters, identifier);
    while (parser->peek_tok->type == COMMA) {
        parser_next_token(parser);
        parser_next_token(parser);
        identifier = create_identifier(parser);
        cm_list_add(function->parameters, identifier);
    }

    if (!expect_peek(parser, RPAREN)) {
        cm_list_free(function->parameters, free_identifier);
        function->parameters = NULL;
        return;
    }
}

static expression_t *
parse_function_literal(parser_t *parser)
{
    #ifdef TRACE
        trace("parse_function_literal");
    #endif
    function_literal_t *function = create_function_literal(parser);
    if (!expect_peek(parser, LPAREN)) {
        free_function_literal(function);
        return NULL;
    }
    parse_function_parameters(parser, function);
    if (function->parameters == NULL) {
        free_function_literal(function);
        return NULL;
    }

    if (!expect_peek(parser, LBRACE)) {
        cm_list_free(function->parameters, free_identifier);
        token_free(function->token);
        free(function);
        return NULL;
    }

    function->body = parse_block_statement(parser);
    #ifdef TRACE
        untrace("parse_function_literal")
    #endif
    return (expression_t *) function;
}

static void
parse_call_arguments(parser_t *parser, call_expression_t *call_exp)
{
    if (parser->peek_tok->type == RPAREN) {
        parser_next_token(parser);
        return;
    }

    parser_next_token(parser);
    expression_t *arg = parse_expression(parser, LOWEST);
    cm_list_add(call_exp->arguments, arg);
    while (parser->peek_tok->type == COMMA) {
        parser_next_token(parser);
        parser_next_token(parser);
        arg = parse_expression(parser, LOWEST);
        cm_list_add(call_exp->arguments, arg);
    }

    if (!expect_peek(parser, RPAREN)) {
        cm_list_free(call_exp->arguments, free_expression);
        call_exp->arguments = NULL;
        return;
    }
}

static expression_t *
parse_call_expression(parser_t *parser, expression_t *function)
{
    #ifdef TRACE
        trace("parse_call_expression");
    #endif
    call_expression_t *call_exp = create_call_expression(parser);
    parse_call_arguments(parser, call_exp);
    if (call_exp->arguments == NULL) {
        free_call_expression(call_exp);
        return NULL;
    }
    call_exp->function = function;
    #ifdef TRACE
        untrace("parse_call_expression");
    #endif
    return (expression_t *) call_exp;
}

static expression_t *
copy_identifier_expression(expression_t *exp)
{
    identifier_t *ident_exp = (identifier_t *) exp;
    identifier_t *copy = malloc(sizeof(*copy));
    if (copy == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->expression.node.string = identifier_string;
    copy->expression.node.token_literal = ident_token_literal;
    copy->expression.node.type = EXPRESSION;
    copy->expression.expression_type = IDENTIFIER_EXPRESSION;
    copy->expression.expression_node = NULL;
    copy->token = token_copy(ident_exp->token);
    copy->value = strdup(ident_exp->value);
    if (copy->value == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    return (expression_t *) copy;
}

static expression_t *
copy_integer_expression(expression_t *exp)
{
    integer_t *int_exp = (integer_t *)exp;
    integer_t *copy = malloc(sizeof(*copy));
    if (copy == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->token = token_copy(int_exp->token);
    copy->expression.expression_node = NULL;
    copy->expression.node.string = integer_string;
    copy->expression.node.token_literal = int_exp_token_literal;
    copy->expression.node.type = EXPRESSION;
    copy->expression.expression_type = INTEGER_EXPRESSION;
    copy->value = int_exp->value;
    return (expression_t *) copy;
}

static expression_t *
copy_prefix_expression(expression_t *exp)
{
    prefix_expression_t *prefix_exp = (prefix_expression_t *)exp;
    prefix_expression_t *copy = malloc(sizeof(*copy));
    if (copy == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->expression.node.string = prefix_exp->expression.node.string;
    copy->expression.node.token_literal = prefix_exp->expression.node.token_literal;
    copy->expression.node.type = EXPRESSION;
    copy->expression.expression_type = PREFIX_EXPRESSION;
    copy->expression.expression_node = NULL;
    copy->token = token_copy(prefix_exp->token);
    copy->operator = strdup(prefix_exp->operator);
    if (copy->operator == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->right = copy_expression(prefix_exp->right);
    return (expression_t *) copy;
}

static expression_t *
copy_infix_expression(expression_t *exp)
{
    infix_expression_t *infix_exp = (infix_expression_t *) exp;
    infix_expression_t *copy = malloc(sizeof(*copy));
    if (copy == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->expression.node.string = infix_exp->expression.node.string;
    copy->expression.node.token_literal = infix_exp->expression.node.token_literal;
    copy->expression.node.type = EXPRESSION;
    copy->expression.expression_type = INFIX_EXPRESSION;
    copy->expression.expression_node = NULL;
    copy->token = token_copy(infix_exp->token);
    copy->operator = strdup(infix_exp->operator);
    if (copy->operator == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->left = copy_expression(infix_exp->left);
    copy->right = copy_expression(infix_exp->right);
    return (expression_t *) copy;
}

static expression_t *
copy_boolean_expression(expression_t *exp)
{
    boolean_expression_t *bool_exp = (boolean_expression_t *) exp;
    boolean_expression_t *copy = malloc(sizeof(*copy));
    if (copy == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->expression.node.string = bool_exp->expression.node.string;
    copy->expression.node.token_literal = bool_exp->expression.node.token_literal;
    copy->expression.node.type = EXPRESSION;
    copy->expression.expression_type = BOOLEAN_EXPRESSION;
    copy->expression.expression_node = NULL;
    copy->token = token_copy(bool_exp->token);
    copy->value = bool_exp->value;
    return (expression_t *) copy;
}

static expression_t *
copy_if_expression(expression_t *exp)
{
    if_expression_t *if_exp = (if_expression_t *) exp;
    if_expression_t *copy = malloc(sizeof(*copy));
    if (copy == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->expression.node.string = if_exp->expression.node.string;
    copy->expression.node.token_literal = if_exp->expression.node.token_literal;
    copy->expression.node.type = EXPRESSION;
    copy->expression.expression_type = IF_EXPRESSION;
    copy->expression.expression_node = NULL;
    copy->token = token_copy(if_exp->token);
    copy->condition = copy_expression(if_exp->condition);
    copy->consequence = (block_statement_t *) copy_statement((statement_t *) if_exp->consequence);
    if (if_exp->alternative)
        copy->alternative = (block_statement_t *) copy_statement((statement_t *) if_exp->alternative);
    else
        copy->alternative = NULL;
    return (expression_t *) copy;
}

cm_list *
copy_parameters(cm_list *parameters)
{
    cm_list *copy_list = cm_list_init();
    cm_list_node *parameters_list_node = parameters->head;
    while (parameters_list_node) {
        identifier_t *param = (identifier_t *) parameters_list_node->data;
        cm_list_add(copy_list, copy_expression((expression_t *) param));
        parameters_list_node = parameters_list_node->next;
    }
    return copy_list;
}

static expression_t *
copy_function_literal(expression_t *exp)
{
    function_literal_t *func = (function_literal_t *) exp;
    function_literal_t *copy = malloc(sizeof(*copy));
    if (copy == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->expression.node.string = func->expression.node.string;
    copy->expression.node.token_literal = func->expression.node.token_literal;
    copy->expression.node.type = EXPRESSION;
    copy->expression.expression_node = NULL;
    copy->expression.expression_type = FUNCTION_LITERAL;
    copy->body = (block_statement_t *) copy_statement((statement_t *) func->body);
    copy->token = token_copy(func->token);
    copy->parameters = copy_parameters(func->parameters);
    return (expression_t *) copy;
}

static expression_t *
copy_call_expression(expression_t *exp)
{
    call_expression_t *call_exp = (call_expression_t *)exp;
    call_expression_t *copy = malloc(sizeof(*copy));
    if (copy == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->expression.node.string = call_exp->expression.node.string;
    copy->expression.node.token_literal = call_exp->expression.node.token_literal;
    copy->expression.node.type = EXPRESSION;
    copy->expression.expression_node = NULL;
    copy->expression.expression_type = CALL_EXPRESSION;
    copy->token = token_copy(call_exp->token);
    copy->arguments = copy_parameters(call_exp->arguments);
    copy->function = copy_expression(call_exp->function);
    return (expression_t *) copy;
}

static expression_t *
copy_string_expression(expression_t *exp)
{
    string_t *string = (string_t *) exp;
    string_t *copy = malloc(sizeof(*copy));
    if (copy == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->length = string->length;
    copy->token = token_copy(string->token);
    copy->expression.node.string = string_string;
    copy->expression.node.token_literal = string_token_literal;
    copy->expression.node.type = EXPRESSION;
    copy->expression.expression_type = STRING_EXPRESSION;
    copy->expression.expression_node = NULL;
    copy->value = strdup(string->value);
    if (copy->value == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    return (expression_t *) copy;
}

static expression_t *
copy_array_literal(expression_t *exp)
{
    array_literal_t *array = (array_literal_t *) exp;
    array_literal_t *copy = malloc(sizeof(*copy));
    if (copy == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->token = token_copy(array->token);
    copy->expression.node.string = array_literal_string;
    copy->expression.node.token_literal = array_literal_token_literal;
    copy->expression.node.type = EXPRESSION;
    copy->expression.expression_type = ARRAY_LITERAL;
    copy->expression.expression_node = NULL;
    copy->elements = cm_array_list_init(array->elements->length, free_expression);
    for (size_t i = 0; i < array->elements->length; i++) {
        cm_array_list_add(copy->elements, copy_expression(array->elements->array[i]));
    }
    return copy;
}

expression_t *
copy_expression(expression_t *exp)
{
    switch (exp->expression_type) {
        case IDENTIFIER_EXPRESSION:
            return copy_identifier_expression(exp);
        case INTEGER_EXPRESSION:
            return copy_integer_expression(exp);
        case PREFIX_EXPRESSION:
            return copy_prefix_expression(exp);
        case INFIX_EXPRESSION:
            return copy_infix_expression(exp);
        case BOOLEAN_EXPRESSION:
            return copy_boolean_expression(exp);
        case IF_EXPRESSION:
            return copy_if_expression(exp);
        case FUNCTION_LITERAL:
            return copy_function_literal(exp);
        case CALL_EXPRESSION:
            return copy_call_expression(exp);
        case STRING_EXPRESSION:
            return copy_string_expression(exp);
        case ARRAY_LITERAL:
            return copy_array_literal(exp);
        default:
            return NULL;
    }
}

static statement_t *
copy_letstatement(statement_t *stmt)
{
    letstatement_t *let_stmt = (letstatement_t *) stmt;
    letstatement_t *copy_stmt = malloc(sizeof(*let_stmt));
    if (copy_stmt == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy_stmt->name = malloc(sizeof(*copy_stmt->name));
    if (copy_stmt->name == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy_stmt->name->token = token_copy(let_stmt->token);
    copy_stmt->name->value = strdup(let_stmt->name->value);
    if (copy_stmt->name->value == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy_stmt->value = copy_expression(let_stmt->value);
    copy_stmt->value = copy_expression(let_stmt->value);
    copy_stmt->statement.node.string = letstatement_string;
    copy_stmt->statement.node.token_literal = letstatement_token_literal;
    copy_stmt->statement.node.type = STATEMENT;
    copy_stmt->statement.statement_type = LET_STATEMENT;
    return (statement_t *)copy_statement;
}

static statement_t *
copy_return_statement(statement_t *stmt)
{
    return_statement_t *ret_stmt = (return_statement_t *) stmt;
    return_statement_t *copy = malloc(sizeof(*copy));
    if (copy == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->token = token_copy(ret_stmt->token);
    copy->statement.node.string = return_statement_string;
    copy->statement.node.token_literal = return_statement_token_literal;
    copy->statement.node.type = STATEMENT;
    copy->statement.statement_type = RETURN_STATEMENT;
    copy->return_value = copy_expression(ret_stmt->return_value);
    return (statement_t *) copy;
}

static statement_t *
copy_expression_statement(statement_t *stmt)
{
    expression_statement_t *exp_stmt = (expression_statement_t *) stmt;
    expression_statement_t *copy = malloc(sizeof(*copy));
    if (copy == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->statement.node.string = exp_stmt->statement.node.string;
    copy->statement.node.token_literal = exp_stmt->statement.node.token_literal;
    copy->statement.node.type = STATEMENT;
    copy->statement.statement_type = EXPRESSION_STATEMENT;
    copy->token = token_copy(exp_stmt->token);
    copy->expression = copy_expression(exp_stmt->expression);
    return (statement_t *) copy;
}

static statement_t *
copy_block_statement(statement_t *stmt)
{
    block_statement_t *block_stmt = (block_statement_t *) stmt;
    block_statement_t *copy = malloc(sizeof(*copy));
    if (copy == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    copy->statement.node.string = block_stmt->statement.node.string;
    copy->statement.node.token_literal = block_stmt->statement.node.token_literal;
    copy->statement.node.type = STATEMENT;
    copy->statement.statement_type = BLOCK_STATEMENT;
    copy->token = token_copy(block_stmt->token);
    copy->nstatements = block_stmt->nstatements;
    copy->array_size = block_stmt->array_size;
    copy->statements = calloc(copy->nstatements, sizeof(*copy->statements));
    if (copy->statements == NULL)
        errx(EXIT_FAILURE, "malloc failed");
    for (size_t i = 0; i < copy->nstatements; i++) {
        copy->statements[i] = copy_statement(block_stmt->statements[i]);
    }
    return (statement_t *) copy;
}


statement_t *
copy_statement(statement_t *stmt)
{
    switch (stmt->statement_type) {
        case LET_STATEMENT:
            return copy_letstatement(stmt);
        case RETURN_STATEMENT:
            return copy_return_statement(stmt);
        case EXPRESSION_STATEMENT:
            return copy_expression_statement(stmt);
        case BLOCK_STATEMENT:
            return copy_block_statement(stmt);
    }
}

