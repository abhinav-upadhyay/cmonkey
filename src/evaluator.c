#include "ast.h"
#include "evaluator.h"
#include "object.h"

static monkey_object_t *
eval_expression(expression_t *exp)
{
    integer_t *int_exp;
    boolean_expression_t *bool_exp;
    switch (exp->expression_type)
    {
        case INTEGER_EXPRESSION:
            int_exp = (integer_t *) exp;
            return (monkey_object_t *) create_monkey_int(int_exp->value);
        case BOOLEAN_EXPRESSION:
            bool_exp = (boolean_expression_t *) exp;
            return (monkey_object_t *) create_monkey_bool(bool_exp->value);
        default:
            break;
    }
    return NULL;
}

static monkey_object_t *
eval_statement(statement_t *statement)
{
    expression_statement_t *exp_stmt;
    switch (statement->statement_type)
    {
        case EXPRESSION_STATEMENT:
            exp_stmt = (expression_statement_t *) statement;
            return eval_expression(exp_stmt->expression);
        default:
            break;
    }
    return NULL;
}


static monkey_object_t *
eval_statements(statement_t **statements, size_t nstatements)
{
    monkey_object_t *object;
    for (size_t i = 0; i < nstatements; i++) {
        object = monkey_eval((node_t *) statements[i]);
    }
    return object;
}


monkey_object_t *
monkey_eval(node_t *node)
{
    program_t *program;
    switch (node->type)
    {
        case STATEMENT:
            return eval_statement((statement_t *) node);
            break;
        case EXPRESSION:
            return eval_expression((expression_t *) node);
        case PROGRAM:
            program = (program_t *) node;
            return eval_statements(program->statements, program->nstatements);
        default:
            break;
    }
}