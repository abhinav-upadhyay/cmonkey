#ifndef PARSER_H
#define PARSER_H
#include "ast.h"
#include "comonkey_utils.h"
#include "lexer.h"
#include "token.h"

typedef struct parser_t {
    lexer_t *lexer;
    token_t *cur_tok;
    token_t *peek_tok;
    cm_list *errors;
 } parser_t;

parser_t * parser_init(lexer_t *);
void parser_next_token(parser_t *);
program_t *parse_program(parser_t *);
statement_t *parser_parse_statement(parser_t *);
void program_free(program_t *);
void parser_free(parser_t *);
void *create_statement(parser_t *, statement_type_t);
void free_statement(void *, statement_type_t);
void _statement_node(void);
void _expression_node(void);

#endif