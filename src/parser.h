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

#ifndef PARSER_H
#define PARSER_H
#include "ast.h"
#include "cmonkey_utils.h"
#include "lexer.h"
#include "token.h"

typedef struct parser_t {
    lexer_t *lexer;
    token_t *cur_tok;
    token_t *peek_tok;
    cm_list *errors;
 } parser_t;

 typedef enum operator_precedence_t {
     LOWEST=1,
     LOGICAL_AND,
     EQUALS,
     LESSGREATER,
     SUM,
     PRODUCT,
     PREFIX,
     CALL,
     INDEX
 } operator_precedence_t;

typedef expression_t * (*prefix_parse_fn)(parser_t *);
typedef expression_t * (*infix_parse_fn)(parser_t *, expression_t *);

parser_t * parser_init(lexer_t *);
void parser_next_token(parser_t *);
program_t *parse_program(parser_t *);
statement_t *parser_parse_statement(parser_t *);
program_t *program_init(void);
void program_free(program_t *);
void parser_free(parser_t *);
void *create_statement(parser_t *, statement_type_t);
void free_statement(statement_t *);
void _statement_node(void);
void _expression_node(void);
char *join_parameters_list(cm_list *);
statement_t *copy_statement(statement_t *);
expression_t *copy_expression(expression_t *);
cm_list *copy_parameters(cm_list *);
void free_expression(void *);
#endif