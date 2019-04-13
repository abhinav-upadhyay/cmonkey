#include <string.h>

#include "ast.h"
#include "evaluator.h"
#include "object.h"

static monkey_object_t *
eval_bang_expression(monkey_object_t *right_value)
{
    if (right_value->type == MONKEY_NULL)
        return (monkey_object_t *) create_monkey_null();
    else if (right_value->type == MONKEY_BOOL) {
        monkey_bool_t *value = (monkey_bool_t *) right_value;
        if (value->value)
            return (monkey_object_t *) create_monkey_bool(false);
        else
            return (monkey_object_t *) create_monkey_bool(true);
    } else
        return (monkey_object_t *) create_monkey_bool(false);
}

static monkey_object_t *
eval_prefix_epxression(const char *operator, monkey_object_t *right_value)
{
    if (strcmp(operator, "!") == 0) {
        return eval_bang_expression(right_value);
    }
    return (monkey_object_t *) create_monkey_null();
}

static monkey_object_t *
eval_expression(expression_t *exp)
{
    integer_t *int_exp;
    boolean_expression_t *bool_exp;
    prefix_expression_t *prefix_exp;
    monkey_object_t *right_value;
    monkey_object_t *prefix_exp_value;
    switch (exp->expression_type)
    {
        case INTEGER_EXPRESSION:
            int_exp = (integer_t *) exp;
            return (monkey_object_t *) create_monkey_int(int_exp->value);
        case BOOLEAN_EXPRESSION:
            bool_exp = (boolean_expression_t *) exp;
            return (monkey_object_t *) create_monkey_bool(bool_exp->value);
        case PREFIX_EXPRESSION:
            prefix_exp = (prefix_expression_t *) exp;
            right_value = monkey_eval((node_t *) prefix_exp->right);
            prefix_exp_value = eval_prefix_epxression(prefix_exp->operator, right_value);
            free_monkey_object(right_value);
            return prefix_exp_value;
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