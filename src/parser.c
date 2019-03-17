#include <err.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "lexer.h"
#include "parser.h"

static char *
program_token_literal(program_t *program)
{
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

void
_statement_node(void)
{

}

void
_expression_node(void)
{
    
}

static char *
identifier_token_literal(identifier_t *id)
{
    return id->token->literal;
}

static letstatement_t *
create_letstatement(parser_t *parser)
{
    letstatement_t *let_stmt;
    let_stmt = malloc(sizeof(*let_stmt));
    if (let_stmt == NULL)
        return NULL;
    let_stmt->token = token_copy(parser->cur_tok);
    if (let_stmt->token == NULL) {
        free(let_stmt);
        return NULL;
    }
    let_stmt->statement.statement_type = LET_STATEMENT;
    let_stmt->statement.node.token_literal = letstatement_token_literal;
    let_stmt->name = NULL;
    let_stmt->value = NULL;
    return let_stmt;
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
            free_statement(stmt, stmt->statement_type);
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
        default:
            return NULL;
    }
}

static void
free_identifier(identifier_t *ident)
{
    token_free(ident->token);
    free(ident->value);
    free(ident);
}

static void
free_letstatement(letstatement_t *let_stmt)
{
    //TODO: free other things also
    if (let_stmt->name)
        free_identifier(let_stmt->name);
    if (let_stmt->token)
        token_free(let_stmt->token);
    free(let_stmt);
}

void
free_statement(void *stmt, statement_type_t stmt_type)
{
    switch (stmt_type)
    {
        case LET_STATEMENT:
            free_letstatement((letstatement_t *) stmt);
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
add_statement(program_t *program, statement_t *stmt)
{
    if (program->nstatements == program->array_size) {
        size_t new_size = program->array_size * 2;
        program->statements = reallocarray(program->statements, new_size, sizeof(*program->statements));
        if (program->statements == NULL)
            return 1;
        program->array_size = new_size;
    }
    program->statements[program->nstatements++] = stmt;
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
    if (parser->errors == NULL) {
        parser->errors = cm_list_init();
        if (parser->errors == NULL)
            errx(EXIT_FAILURE, "malloc failed");
    }
    cm_list_add(parser->errors, msg);
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

//TODO: can we create a create_expression API like we have for statements?
static identifier_t *
parse_identifier(parser_t * parser)
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
    ident->node.token_literal = ident_token_literal;
    ident->value = strdup(parser->cur_tok->literal);
    if (ident->value == NULL) {
        token_free(ident->token);
        free(ident);
        errx(EXIT_FAILURE, "malloc failed");
    }
    return ident;
}

static letstatement_t *
parse_letstatement(parser_t *parser)
{
    letstatement_t *let_stmt = (letstatement_t *) create_statement(parser, LET_STATEMENT);
    if (let_stmt == NULL)
        errx(EXIT_FAILURE, "malloc failed"); // returning NULL would indicate no valid token
    
    if (!expect_peek(parser, IDENT)) {
        free_statement(let_stmt, LET_STATEMENT);
        return NULL;
    }

    identifier_t *ident = parse_identifier(parser);
    let_stmt->name = ident;
    if (!expect_peek(parser, ASSIGN)) {
        free_statement(let_stmt, LET_STATEMENT);
        return NULL;
    }

    while (parser->cur_tok->type != SEMICOLON)
        parser_next_token(parser);

    return let_stmt;
}

program_t *
parse_program(parser_t *parser)
{
    program_t *program;
    program = malloc(sizeof(*program));
    if (program == NULL)
        return NULL;
    program->array_size = 64;
    program->statements = calloc(64, sizeof(*program->statements));
    if (program->statements == NULL) {
        free(program);
        return NULL;
    }
    program->nstatements = 0;

    while (parser->cur_tok->type != END_OF_FILE) {
        statement_t *stmt = parser_parse_statement(parser);
        if (stmt != NULL) {
            int status = add_statement(program, stmt);
            if (status != 0) {
                program_free(program);
                return NULL;
            }
        }
        parser_next_token(parser);
    }
    return program;
}

statement_t *
parser_parse_statement(parser_t *parser)
{
    letstatement_t *let_stmt;
    switch(parser->cur_tok->type) {
        case LET:
            let_stmt = parse_letstatement(parser);
            return (statement_t *) let_stmt;
        default:
            return NULL;
    }
}

