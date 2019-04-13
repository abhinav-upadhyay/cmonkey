#include <stdbool.h>
#include <string.h>

#include "ast.h"
#include "evaluator.h"
#include "object.h"

static _Bool
is_error(monkey_object_t *obj)
{
    return obj != NULL && obj->type == MONKEY_ERROR;
}

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
        return (monkey_object_t *) create_monkey_error("unknown operator: %s %s %s",
            get_type_name(left_value->object.type), operator,
            get_type_name(right_value->object.type));
    return (monkey_object_t *) create_monkey_int(result);
}

static monkey_object_t *
eval_minus_prefix_expression(monkey_object_t *right_value)
{
    if (right_value->type != MONKEY_INT)
        return (monkey_object_t *) create_monkey_error("unknown operator: -%s",
            get_type_name(right_value->type));
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
        return eval_minus_prefix_expression(right_value);
    }
    return (monkey_object_t *) create_monkey_error("unknown operator: %s%s",
        operator, get_type_name(right_value->type));
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
    else if (strcmp(operator, "==") == 0)
        return (monkey_object_t *)
            create_monkey_bool(left_value == right_value);
    else if (strcmp(operator, "!=") == 0)
        return (monkey_object_t *)
            create_monkey_bool(left_value != right_value);
    else if (left_value->type != right_value->type)
        return (monkey_object_t *) create_monkey_error("type mismatch: %s %s %s",
            get_type_name(left_value->type), operator, get_type_name(right_value->type));
    else
        return (monkey_object_t *) create_monkey_error("unknown operator: %s %s %s",
            get_type_name(left_value->type), operator, get_type_name(right_value->type));
    return (monkey_object_t *) create_monkey_null();
}

static _Bool
is_truthy(monkey_object_t *value)
{
    switch (value->type) {
        case MONKEY_NULL:
            return false;
        case MONKEY_BOOL:
            return create_monkey_bool(true) == (monkey_bool_t *) value;
        default:
            return true;
    }
}

static monkey_object_t *
eval_if_expression(expression_t *exp)
{
    if_expression_t *if_exp = (if_expression_t *) exp;
    monkey_object_t *condition_value = monkey_eval((node_t *) if_exp->condition);
    if (is_error(condition_value))
        return condition_value;
    monkey_object_t *result;
    if (is_truthy(condition_value)) {
        result = monkey_eval((node_t *) if_exp->consequence);
    } else if (if_exp->alternative != NULL)
        result = monkey_eval((node_t *) if_exp->alternative);
    else
        result = (monkey_object_t *) create_monkey_null();
    free_monkey_object(condition_value);
    return result;
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
            if (is_error(right_value))
                return right_value;
            exp_value = eval_prefix_epxression(prefix_exp->operator, right_value);
            free_monkey_object(right_value);
            return exp_value;
        case INFIX_EXPRESSION:
            infix_exp = (infix_expression_t *) exp;
            left_value = monkey_eval((node_t *) infix_exp->left);
            if (is_error(left_value))
                return left_value;
            right_value = monkey_eval((node_t *) infix_exp->right);
            if (is_error(right_value)) {
                free_monkey_object(left_value);
                return right_value;
            }
            exp_value = eval_infix_expression(infix_exp->operator, left_value, right_value);
            free_monkey_object(left_value);
            free_monkey_object(right_value);
            return exp_value;
        case IF_EXPRESSION:
            return eval_if_expression(exp);
        default:
            break;
    }
    return NULL;
}

static monkey_object_t *
eval_block_statement(block_statement_t *block_stmt)
{
    monkey_object_t *object = NULL;
    for (size_t i = 0; i < block_stmt->nstatements; i++) {
        if (object)
            free_monkey_object(object);
        object = monkey_eval((node_t *) block_stmt->statements[i]);
        if (object != NULL &&
            (object->type == MONKEY_RETURN_VALUE ||
            object->type == MONKEY_ERROR)) {
            return object;
        }
    }
    return object;
}

static monkey_object_t *
eval_program(program_t *program)
{
    monkey_object_t *object = NULL;
    monkey_return_value_t *return_value_object;
    monkey_object_t *ret_value;
    for (size_t i = 0; i < program->nstatements; i++) {
        if (object)
            free_monkey_object(object);
        object = monkey_eval((node_t *) program->statements[i]);
        if (object->type == MONKEY_RETURN_VALUE) {
            return_value_object = (monkey_return_value_t *) object;
            ret_value = return_value_object->value;
            free_monkey_object((monkey_object_t *) return_value_object);
            return ret_value;
        } else if (object->type == MONKEY_ERROR)
            return object;
    }
    return object;
}

static monkey_object_t *
eval_statement(statement_t *statement)
{
    expression_statement_t *exp_stmt;
    block_statement_t *block_stmt;
    return_statement_t *ret_stmt;
    monkey_object_t *evaluated;
    switch (statement->statement_type)
    {
        case EXPRESSION_STATEMENT:
            exp_stmt = (expression_statement_t *) statement;
            return eval_expression(exp_stmt->expression);
        case BLOCK_STATEMENT:
            block_stmt = (block_statement_t *) statement;
            return eval_block_statement(block_stmt);
        case RETURN_STATEMENT:
            ret_stmt = (return_statement_t *) statement;
            evaluated = monkey_eval((node_t *) ret_stmt->return_value);
            if (is_error(evaluated))
                return evaluated;
            return (monkey_object_t *)create_monkey_return_value(evaluated);
        default:
            break;
    }
    return NULL;
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
            return eval_program(program);
        default:
            break;
    }
}