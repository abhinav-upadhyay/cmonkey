#include <string.h>

#include "ast.h"
#include "evaluator.h"
#include "object.h"

static monkey_object_t *
eval_integer_infix_expression(const char *operator,
    monkey_int_t *left_value,
    monkey_int_t *right_value)
{
    long result;
    if (strcmp(operator, "+") == 0)
        result = left_value->value + right_value->value;
    else if (strcmp(operator, "-") == 0)
        result = left_value->value - right_value->value;
    else if (strcmp(operator, "*") == 0)
        result = left_value->value * right_value->value;
    else if (strcmp(operator, "/") == 0)
        result = left_value->value / right_value->value;
    else if (strcmp(operator, "<") == 0)
        return (monkey_object_t *)
            create_monkey_bool(left_value->value < right_value->value);
    else if (strcmp(operator, ">") == 0)
        return (monkey_object_t *)
            create_monkey_bool(left_value->value > right_value->value);
    else if (strcmp(operator, "==") == 0)
        return (monkey_object_t *)
            create_monkey_bool(left_value->value == right_value->value);
    else if (strcmp(operator, "!=") == 0)
        return (monkey_object_t *)
            create_monkey_bool(left_value->value != right_value->value);
    else
        return (monkey_object_t *) create_monkey_null;
    return (monkey_object_t *) create_monkey_int(result);
}

static monkey_object_t *
eval_minus_expression(monkey_object_t *right_value)
{
    if (right_value->type != MONKEY_INT)
        return (monkey_object_t *) create_monkey_null();
    monkey_int_t *int_obj = (monkey_int_t *) right_value;
    return (monkey_object_t *) create_monkey_int(-(int_obj->value));
}

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
    } else if (strcmp(operator, "-") == 0) {
        return eval_minus_expression(right_value);
    }
    return (monkey_object_t *) create_monkey_null();
}

static monkey_object_t *
eval_infix_expression(const char *operator,
    monkey_object_t *left_value,
    monkey_object_t *right_value)
{
    if (left_value->type == MONKEY_INT && right_value->type == MONKEY_INT)
        return eval_integer_infix_expression(operator,
            (monkey_int_t *) left_value,
            (monkey_int_t *) right_value);
    return (monkey_object_t *) create_monkey_null();
}

static monkey_object_t *
eval_expression(expression_t *exp)
{
    integer_t *int_exp;
    boolean_expression_t *bool_exp;
    prefix_expression_t *prefix_exp;
    infix_expression_t *infix_exp;
    monkey_object_t *left_value;
    monkey_object_t *right_value;
    monkey_object_t *exp_value;
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
            exp_value = eval_prefix_epxression(prefix_exp->operator, right_value);
            free_monkey_object(right_value);
            return exp_value;
        case INFIX_EXPRESSION:
            infix_exp = (infix_expression_t *) exp;
            left_value = monkey_eval((node_t *) infix_exp->left);
            right_value = monkey_eval((node_t *) infix_exp->right);
            exp_value = eval_infix_expression(infix_exp->operator, left_value, right_value);
            free_monkey_object(left_value);
            free_monkey_object(right_value);
            return exp_value;
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