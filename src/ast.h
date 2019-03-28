#ifndef AST_H
#define AST_H

#include <stdio.h>
#include "token.h"

typedef enum node_type_t {
    STATEMENT,
    EXPRESSION
} node_type_t;

typedef enum statement_type_t {
    LET_STATEMENT,
    RETURN_STATEMENT,
    EXPRESSION_STATEMENT
} statement_type_t;

static const char *statement_type_values[] = {
    "LET_STATEMENT",
    "RETURN_STATEMENT",
    "EXPRESSION_STATEMENT"
};

typedef enum expression_type_t {
    IDENTIFIER_EXPRESSION
} expression_type_t;

static const char *expression_type_values[] = {
    "IDENTIFIER_EXPRESSION"
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

#define get_statement_type_name(type) statement_type_values[type]
#define get_expression_type_name(type) expression_type_values[type]

#endif