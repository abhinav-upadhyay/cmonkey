#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stdio.h>

#include "cmonkey_utils.h"
#include "token.h"

typedef enum node_type_t {
    STATEMENT,
    EXPRESSION
} node_type_t;

typedef enum statement_type_t {
    LET_STATEMENT,
    RETURN_STATEMENT,
    EXPRESSION_STATEMENT,
    BLOCK_STATEMENT
} statement_type_t;

static const char *statement_type_values[] = {
    "LET_STATEMENT",
    "RETURN_STATEMENT",
    "EXPRESSION_STATEMENT",
    "BLOCK_STATEMENT"
};

typedef enum expression_type_t {
    IDENTIFIER_EXPRESSION,
    INTEGER_EXPRESSION,
    PREFIX_EXPRESSION,
    INFIX_EXPRESSION,
    BOOLEAN_EXPRESSION,
    IF_EXPRESSION,
    FUNCTION_LITERAL
} expression_type_t;

static const char *expression_type_values[] = {
    "IDENTIFIER_EXPRESSION",
    "INTEGER_EXPRESSION",
    "PREFIX_EXPRESSION",
    "INFIX_EXPRESSION",
    "BOOLEAN_EXPRESSION",
    "IF_EXPRESSION",
    "FUNCTION_LITERAL"
};

typedef struct node_t {
    char * (*token_literal) (void *); // return token literal for the node
    char * (*string) (void *);
} node_t;

typedef struct statement_t {
    node_t node;
    statement_type_t statement_type;
} statement_t;

typedef struct expression_t {
    node_t node;
    expression_type_t expression_type;
    void (*expression_node) (void); // TODO: do we need this?
} expression_t;

typedef struct program_t {
    node_t node;
    statement_t **statements; //array of statements
    size_t nstatements; // number of statements
    size_t array_size; //size of statements array so that we can grow it as required
} program_t;

typedef struct identifier_t {
    expression_t expression;
    token_t *token;
    char *value;
} identifier_t;

typedef struct integer_t {
    expression_t expression;
    token_t *token;
    long value;
} integer_t;

typedef struct prefix_expression_t {
    expression_t expression;
    token_t *token;
    expression_t *right;
    char *operator;
} prefix_expression_t;

typedef struct infix_expression_t {
    expression_t expression;
    token_t *token;
    expression_t *left;
    expression_t *right;
    char *operator;
} infix_expression_t;

typedef struct letstatement_t {
    statement_t statement;
    token_t *token;
    identifier_t *name;
    expression_t *value;
} letstatement_t;

typedef struct return_statement_t {
    statement_t statement;
    token_t *token;
    expression_t *return_value;
} return_statement_t;

typedef struct expression_statement_t {
    statement_t statement;
    token_t *token;
    expression_t *expression;
} expression_statement_t;

typedef struct block_statement_t {
    statement_t statement;
    token_t *token;
    statement_t **statements; //array of statements
    size_t nstatements; // number of statements
    size_t array_size; //size of statements array so that we can grow it as required
} block_statement_t;

typedef struct boolean_expression_t {
    expression_t expression;
    token_t *token;
    _Bool value;
} boolean_expression_t;

typedef struct if_expression_t {
    expression_t expression;
    token_t *token;
    expression_t *condition;
    block_statement_t *consequence;
    block_statement_t *alternative;
} if_expression_t;

typedef struct function_literal_t {
    expression_t expression;
    token_t *token;
    size_t nparameters;
    cm_list *parameters;
    block_statement_t *body;
} function_literal_t;

#define get_statement_type_name(type) statement_type_values[type]
#define get_expression_type_name(type) expression_type_values[type]

#endif