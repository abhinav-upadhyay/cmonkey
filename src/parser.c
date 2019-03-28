#include <err.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "lexer.h"
#include "parser.h"

static expression_t * parse_identifier(parser_t *);


 static prefix_parse_fn prefix_fns [] = {
     NULL, //ILLEGAL
     NULL, //END OF FILE
     parse_identifier, //IDENT
     NULL, //INT
     NULL, //ASSIGN
     NULL, //PLUS
     NULL, //MINUS
     NULL, //BANG
     NULL, //SLASH
     NULL, //ASTERISK
     NULL, //LT
     NULL, //GT
     NULL, //EQ
     NULL, //NOT_EQ
     NULL, //COMMA
     NULL, //SEMICOLON
     NULL, //LPAREN
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

 static infix_parse_fn infix_fns [] = {
     NULL, //ILLEGAL
     NULL, //END OF FILE
     NULL, //IDENT
     NULL, //INT
     NULL, //ASSIGN
     NULL, //PLUS
     NULL, //MINUS
     NULL, //BANG
     NULL, //SLASH
     NULL, //ASTERISK
     NULL, //LT
     NULL, //GT
     NULL, //EQ
     NULL, //NOT_EQ
     NULL, //COMMA
     NULL, //SEMICOLON
     NULL, //LPAREN
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
identifier_token_literal(identifier_t *id)
{
    return id->token->literal;
}

static char *
expression_statement_token_literal(void *stmt)
{
    expression_statement_t *exp_stmt = (expression_statement_t *) stmt;
    return exp_stmt->token->literal;
}

void
_expression_node(void)
{
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
expression_statement_string(void *stmt)
{
    expression_statement_t *exp_stmt = (expression_statement_t *) stmt;
    if (exp_stmt->expression) {
        return (exp_stmt->expression->node.string(exp_stmt->expression));
    }
    return strdup("");
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
        if (prog_string != NULL)
            asprintf(&temp_string, "%s %s", prog_string, stmt_string);
        else
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
    return exp_stmt;
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
        case RETURN_STATEMENT:
            return create_return_statement(parser);
        case EXPRESSION_STATEMENT:
            return create_expression_statement(parser);
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
free_return_statement(return_statement_t *ret_stmt)
{
    if (ret_stmt->return_value)
        free(ret_stmt->return_value); //TODO: we need expression specific free functions
    if (ret_stmt->token)
        token_free(ret_stmt->token);
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
    free(let_stmt);
}

static void
free_expression(expression_t *exp)
{
    switch (exp->expression_type)
    {
        case IDENTIFIER_EXPRESSION:
            free_identifier((identifier_t *) exp);
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

void
free_statement(void *stmt, statement_type_t stmt_type)
{
    switch (stmt_type)
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
        program->statements = reallocarray(program->statements,
            new_size, sizeof(*program->statements));
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
static expression_t *
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
    ident->expression.node.token_literal = ident_token_literal;
    ident->expression.expression_type = IDENTIFIER_EXPRESSION;
    ident->expression.node.string = identifier_string;
    ident->value = strdup(parser->cur_tok->literal);
    if (ident->value == NULL) {
        token_free(ident->token);
        free(ident);
        errx(EXIT_FAILURE, "malloc failed");
    }
    return (expression_t *) ident;
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

    identifier_t *ident = (identifier_t *) parse_identifier(parser);
    let_stmt->name = ident;
    if (!expect_peek(parser, ASSIGN)) {
        free_statement(let_stmt, LET_STATEMENT);
        return NULL;
    }

    while (parser->cur_tok->type != SEMICOLON)
        parser_next_token(parser);

    return let_stmt;
}

static return_statement_t *
parse_return_statement(parser_t *parser)
{
    return_statement_t *ret_stmt = (return_statement_t *)
        create_statement(parser, RETURN_STATEMENT);
    while (parser->cur_tok->type != SEMICOLON)
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

static expression_t *
parser_parse_expression(parser_t * parser, operator_precedence_t precedence)
{
    prefix_parse_fn prefix_fn = prefix_fns[parser->cur_tok->type];
    if (prefix_fn == NULL)
        return NULL;
    expression_t *left_exp = prefix_fn(parser);
    return left_exp;
}

static expression_statement_t *
parse_expression_statement(parser_t *parser)
{
    expression_statement_t *exp_stmt = create_expression_statement(parser);
    exp_stmt->expression = parser_parse_expression(parser, LOWEST);
    if (parser->peek_tok->type == SEMICOLON)
        parser_next_token(parser);
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

