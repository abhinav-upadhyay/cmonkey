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

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "ast.h"
#include "builtins.h"
#include "environment.h"
#include "evaluator.h"
#include "object.h"

static _Bool
is_error(monkey_object_t *obj)
{
    return obj != NULL && obj->type == MONKEY_ERROR;
}

static monkey_object_t *
eval_boolean_infix_expression(const char *operator,
    monkey_bool_t *left_value, monkey_bool_t *right_value)
{
    _Bool result;
    if (strcmp(operator, "&&") == 0)
        result = left_value->value && right_value->value;
    else if (strcmp(operator, "||") == 0)
        result = left_value->value || right_value->value;
    else if (strcmp(operator, "==") == 0)
        result = left_value->value == right_value->value;
    else if (strcmp(operator, "!=") == 0)
        result = left_value->value != right_value->value;
    else
        return (monkey_object_t *) create_monkey_error("unknown operator: %s %s %s",
            get_type_name(left_value->object.type), operator,
            get_type_name(right_value->object.type));
    return (monkey_object_t *) create_monkey_bool(result);
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
        if (right_value->value != 0)
            result = left_value->value / right_value->value;
        else
            return (monkey_object_t *) create_monkey_error("division by 0 not allowed");
    else if (strcmp(operator, "<") == 0)
        return (monkey_object_t *)
            create_monkey_bool(left_value->value < right_value->value);
    else if (strcmp(operator, ">") == 0)
        return (monkey_object_t *)
            create_monkey_bool(left_value->value > right_value->value);
    else if (strcmp(operator, "==") == 0)
        return (monkey_object_t *)
            (monkey_object_t *) create_monkey_bool(left_value->value == right_value->value);
    else if (strcmp(operator, "!=") == 0)
        return (monkey_object_t *)
            create_monkey_bool(left_value->value != right_value->value);
    else if (strcmp(operator, "%") == 0)
    if (right_value->value != 0)
        result = left_value->value % right_value->value;
    else
        return (monkey_object_t *) create_monkey_error("division by 0 not allowed");
    else
        return (monkey_object_t *) create_monkey_error("unknown operator: %s %s %s",
            get_type_name(left_value->object.type), operator,
            get_type_name(right_value->object.type));
    return (monkey_object_t *) create_monkey_int(result);
}

static monkey_object_t *
eval_string_infix_expression(const char *operator,
    monkey_string_t *left_value,
    monkey_string_t *right_value)
{
    if (strcmp(operator, "+") == 0) {
        size_t new_len = left_value->length + right_value->length;
        char *new_string = malloc(new_len + 1);
        memcpy(new_string, left_value->value, left_value->length);
        memcpy(new_string + left_value->length, right_value->value, right_value->length);
        new_string[new_len] = 0;
        monkey_string_t *new_string_obj = create_monkey_string(NULL, 0);
        new_string_obj->value = new_string;
        new_string_obj->length = new_len;
        return (monkey_object_t *) new_string_obj;
    }

    if (strcmp(operator, "==") == 0) {
        if (strcmp(left_value->value, right_value->value) == 0)
            return (monkey_object_t *) create_monkey_bool(true);
        else
            return (monkey_object_t *) create_monkey_bool(false);
    }

    if (strcmp(operator, "!=") == 0) {
        if (strcmp(left_value->value, right_value->value) == 0)
            return (monkey_object_t *) create_monkey_bool(false);
        else
            return (monkey_object_t *) create_monkey_bool(true);
    }

    return (monkey_object_t *) create_monkey_error("unknown operator: %s %s %s",
            get_type_name(left_value->object.type),
            operator,
            get_type_name(right_value->object.type));
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
    if (left_value->type == MONKEY_STRING && right_value->type == MONKEY_STRING)
        return eval_string_infix_expression(operator,
            (monkey_string_t *) left_value,
            (monkey_string_t *) right_value);
    if (left_value->type == MONKEY_BOOL && right_value->type == MONKEY_BOOL)
        return eval_boolean_infix_expression(operator,
            (monkey_bool_t *) left_value,
            (monkey_bool_t *) right_value);
    if (strcmp(operator, "==") == 0)
        return (monkey_object_t *)
            create_monkey_bool(left_value == right_value);
    if (strcmp(operator, "!=") == 0)
        return (monkey_object_t *)
            create_monkey_bool(left_value != right_value);
    if (left_value->type != right_value->type)
        return (monkey_object_t *) create_monkey_error("type mismatch: %s %s %s",
            get_type_name(left_value->type), operator, get_type_name(right_value->type));
    else
        return (monkey_object_t *) create_monkey_error("unknown operator: %s %s %s",
            get_type_name(left_value->type), operator, get_type_name(right_value->type));
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
eval_if_expression(expression_t *exp, environment_t *env)
{
    if_expression_t *if_exp = (if_expression_t *) exp;
    monkey_object_t *condition_value = monkey_eval((node_t *) if_exp->condition, env);
    if (is_error(condition_value))
        return condition_value;
    monkey_object_t *result;
    if (is_truthy(condition_value))
        result = monkey_eval((node_t *) if_exp->consequence, env);
    else if (if_exp->alternative != NULL)
        result = monkey_eval((node_t *) if_exp->alternative, env);
    else
        result = (monkey_object_t *) create_monkey_null();
    free_monkey_object(condition_value);
    return result;
}

static monkey_object_t *
eval_identifier_expression(expression_t *exp, environment_t *env)
{
    identifier_t *ident_exp = (identifier_t *) exp;
    void *value_obj = env_get(env, ident_exp->value);
    if (value_obj == NULL)
        value_obj = (void *) get_builtins(ident_exp->value);
    if (value_obj == NULL)
        return (monkey_object_t *) create_monkey_error("identifier not found: %s", ident_exp->value);
    // return a copy of the value, we don't want anyone else to a value stored in the hash table
    return copy_monkey_object((monkey_object_t *) value_obj);
}

static cm_array_list *
eval_expressions_to_array_list(cm_array_list *expression_list, environment_t *env)
{
    cm_array_list *values = cm_array_list_init(expression_list->length, NULL);
    monkey_object_t *value;
    for (size_t i = 0; i < expression_list->length; i++) {
        value = monkey_eval((node_t *) expression_list->array[i], env);
        if (is_error(value)) {
            cm_array_list_free(values);
            values = cm_array_list_init(1, free_monkey_object);
            cm_array_list_add(values, value);
            return values;
        }
        cm_array_list_add(values, value);
    }
    return values;
}

static cm_list *
eval_expressions_to_linked_list(cm_list *expression_list, environment_t *env)
{
    cm_list *values = cm_list_init();
    monkey_object_t *value;
    cm_list_node *exp_node = expression_list->head;
    while (exp_node != NULL) {
        value = monkey_eval((node_t *) exp_node->data, env);
        if (is_error(value)) {
            cm_list_free(values, free_monkey_object);
            values = cm_list_init();
            cm_list_add(values, value);
            return values;
        }
        cm_list_add(values, value);
        exp_node = exp_node->next;
    }
    return values;
}

static monkey_object_t *
apply_function(monkey_object_t *function_obj, cm_list *arguments_list)
{
    monkey_function_t *function;
    monkey_builtin_t *builtin;
    environment_t *extended_env;
    monkey_object_t *function_value;
    monkey_return_value_t *ret_value;
    monkey_object_t *ret;
    cm_list_node *arg_node;
    cm_list_node *param_node;

    switch (function_obj->type) {
        case MONKEY_FUNCTION:
            function = (monkey_function_t *) function_obj;
            extended_env = create_enclosed_env(function->env);
            arg_node = arguments_list->head;
            param_node = function->parameters->head;
            while (arg_node != NULL) {
                identifier_t *param = (identifier_t *) param_node->data;
                env_put(extended_env, strdup(param->value), copy_monkey_object(arg_node->data));
                arg_node = arg_node->next;
                param_node = param_node->next;
            }
            function_value = monkey_eval((node_t *) function->body, extended_env);
            env_free(extended_env);
            if (function_value->type == MONKEY_RETURN_VALUE) {
                ret_value = (monkey_return_value_t *) function_value;
                ret = copy_monkey_object(ret_value->value);
                free_monkey_object(ret_value);
                return ret;
            }
            return function_value;
            break;
        case MONKEY_BUILTIN:
            builtin = (monkey_builtin_t *) function_obj;
            return builtin->function(arguments_list);
            break;
        default:
            return (monkey_object_t *) create_monkey_error("not a function: %s", get_type_name(function_obj->type));
    }
}

static monkey_object_t *
eval_array_index_expression(monkey_object_t *left_value, monkey_object_t *index_value)
{
    monkey_array_t *array_obj = (monkey_array_t *) left_value;
    monkey_int_t *index_obj = (monkey_int_t *) index_value;
    if (index_obj->value < 0 || index_obj->value > array_obj->elements->length - 1) {
        return (monkey_object_t *) create_monkey_null();
    }

    /* we need to copy the return value because the left_value and index_value objects need to be freed */
    return (monkey_object_t *) copy_monkey_object(array_obj->elements->array[index_obj->value]);
}

static monkey_object_t *
eval_string_index_expression(monkey_object_t *left_value, monkey_object_t *index_value)
{
    monkey_string_t *string = (monkey_string_t *) left_value;
    monkey_int_t *index = (monkey_int_t *) index_value;
    if (index->value < 0 || index->value > string->length - 1) {
        return (monkey_object_t *) create_monkey_null();
    }
    return (monkey_object_t *) create_monkey_string(&string->value[index->value], 1);
}

static monkey_object_t *
eval_hash_index_expression(monkey_object_t *left_value, monkey_object_t *index_value)
{
    monkey_hash_t *hash_obj = (monkey_hash_t *) left_value;
    if (index_value->hash == NULL) {
        return (monkey_object_t *) create_monkey_error("unusable as a hash key: %s",
            get_type_name(index_value->type));
    }
    return copy_monkey_object(cm_hash_table_get(hash_obj->pairs, index_value));
}

static monkey_object_t *
eval_index_expression(monkey_object_t *left_value, monkey_object_t *index_value)
{
    if (left_value->type == MONKEY_ARRAY && index_value->type == MONKEY_INT) {
        return eval_array_index_expression(left_value, index_value);
    } else if (left_value->type == MONKEY_HASH) {
        return eval_hash_index_expression(left_value, index_value);
    } else if(left_value->type == MONKEY_STRING && index_value->type == MONKEY_INT) {
        return eval_string_index_expression(left_value, index_value);
    } else {
        return (monkey_object_t *) create_monkey_error("index operator not supported: %s",
            get_type_name(left_value->type));
    }
}

static monkey_object_t *
eval_while_expression(while_expression_t *while_exp, environment_t *env)
{
    monkey_object_t *result = NULL;
    monkey_object_t *condition = monkey_eval((node_t *) while_exp->condition, env);
    if (is_error(condition))
        return condition;
    while (is_truthy(condition)) {
        result = monkey_eval((node_t *) while_exp->body, env);
        if (is_error(result)) {
            free_monkey_object(condition);
            return result;
        }
        free_monkey_object(condition);
        condition = monkey_eval((node_t *) while_exp->condition, env);
        if (is_truthy(condition))
            free_monkey_object(result);
    }
    if (result == NULL)
        return (monkey_object_t *) create_monkey_null();
    return result;
}

static monkey_object_t *
eval_hash_literal(hash_literal_t *hash_exp, environment_t *env)
{
    cm_hash_table *pairs = cm_hash_table_init(monkey_object_hash,
    monkey_object_equals, NULL, NULL);
    cm_array_list *keys = cm_hash_table_get_keys(hash_exp->pairs);
    if (keys != NULL) {
        for (size_t i = 0; i < keys->length; i++) {
            expression_t *exp_key = (expression_t *) cm_array_list_get(keys, i);
            expression_t *exp_value = (expression_t *) cm_hash_table_get(hash_exp->pairs, exp_key);
            monkey_object_t *key = monkey_eval((node_t *) exp_key, env);
            if (is_error(key)) {
                cm_hash_table_free(pairs);
                return key;
            }
            if (key->hash == NULL) {
                cm_hash_table_free(pairs);
                return (monkey_object_t *)
                    create_monkey_error("unusable as a hash key: %s",
                    get_type_name(key->type));
            }
            monkey_object_t *value = monkey_eval((node_t *) exp_value, env);
            if (is_error(value)) {
                free_monkey_object(key);
                cm_hash_table_free(pairs);
                return value;
            }
            cm_hash_table_put(pairs, key, value);
        }
        cm_array_list_free(keys);
    }
    return (monkey_object_t *) create_monkey_hash(pairs);
}

static monkey_object_t *
eval_expression(expression_t *exp, environment_t *env)
{
    integer_t *int_exp;
    boolean_expression_t *bool_exp;
    prefix_expression_t *prefix_exp;
    infix_expression_t *infix_exp;
    monkey_object_t *left_value;
    monkey_object_t *right_value;
    monkey_object_t *exp_value;
    monkey_object_t *function_value;
    monkey_object_t *call_exp_value;
    monkey_object_t *index_exp_value;
    cm_list *arguments_value;
    function_literal_t *function_exp;
    call_expression_t *call_exp;
    string_t *string_exp;
    array_literal_t *array_exp;
    index_expression_t *index_exp;
    hash_literal_t *hash_exp;
    while_expression_t *while_exp;

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
            right_value = monkey_eval((node_t *) prefix_exp->right, env);
            if (is_error(right_value))
                return right_value;
            exp_value = eval_prefix_epxression(prefix_exp->operator, right_value);
            free_monkey_object(right_value);
            return exp_value;
        case INFIX_EXPRESSION:
            infix_exp = (infix_expression_t *) exp;
            left_value = monkey_eval((node_t *) infix_exp->left, env);
            if (is_error(left_value))
                return left_value;
            right_value = monkey_eval((node_t *) infix_exp->right, env);
            if (is_error(right_value)) {
                free_monkey_object(left_value);
                return right_value;
            }
            exp_value = eval_infix_expression(infix_exp->operator, left_value, right_value);
            free_monkey_object(left_value);
            free_monkey_object(right_value);
            return exp_value;
        case IF_EXPRESSION:
            return eval_if_expression(exp, env);
        case IDENTIFIER_EXPRESSION:
            return eval_identifier_expression(exp, env);
        case FUNCTION_LITERAL:
            function_exp = (function_literal_t *) exp;
            return (monkey_object_t *) create_monkey_function(
                    function_exp->parameters,
                    function_exp->body,
                    env);
        case CALL_EXPRESSION:
            call_exp = (call_expression_t *) exp;
            function_value = monkey_eval((node_t *) call_exp->function, env);
            if (is_error(function_value)) {
                return function_value;
            }
            arguments_value = eval_expressions_to_linked_list(call_exp->arguments, env);
            if (arguments_value->length == 1 &&
                is_error((monkey_object_t *) arguments_value->head->data)) {
                    free_monkey_object(function_value);
                    exp_value = copy_monkey_object((monkey_object_t *) arguments_value->head->data);
                    cm_list_free(arguments_value, free_monkey_object);
                    return exp_value;
            }
            call_exp_value = apply_function(function_value, arguments_value);
            free_monkey_object(function_value);
            cm_list_free(arguments_value, free_monkey_object);
            return call_exp_value;
        case STRING_EXPRESSION:
            string_exp = (string_t *) exp;
            return (monkey_object_t *) create_monkey_string(string_exp->value, string_exp->length);
        case ARRAY_LITERAL:
            array_exp = (array_literal_t *) exp;
            cm_array_list *elements = eval_expressions_to_array_list(array_exp->elements, env);
            if (elements->length == 1 && is_error(elements->array[0])) {
                free_monkey_object(array_exp);
                exp_value = copy_monkey_object((monkey_object_t *) elements->array[0]);
                cm_array_list_free(elements);
                return exp_value;
            }
            return (monkey_object_t *) create_monkey_array(elements);
        case INDEX_EXPRESSION:
            index_exp = (index_expression_t *) exp;
            left_value = monkey_eval((node_t *) index_exp->left, env);
            if (is_error(left_value))
                return left_value;
            exp_value = monkey_eval((node_t *) index_exp->index, env);
            if (is_error(exp_value)) {
                free_monkey_object(left_value);
                return exp_value;
            }
            index_exp_value = eval_index_expression(left_value, exp_value);
            free_monkey_object(left_value);
            free_monkey_object(exp_value);
            return index_exp_value;
        case HASH_LITERAL:
            hash_exp = (hash_literal_t *) exp;
            return eval_hash_literal(hash_exp, env);
        case WHILE_EXPRESSION:
            while_exp = (while_expression_t *) exp;
            return eval_while_expression(while_exp, env);
        default:
            break;
    }
    return NULL;
}

static monkey_object_t *
eval_block_statement(block_statement_t *block_stmt, environment_t *env)
{
    monkey_object_t *object = NULL;
    for (size_t i = 0; i < block_stmt->nstatements; i++) {
        if (object)
            free_monkey_object(object);
        object = monkey_eval((node_t *) block_stmt->statements[i], env);
        if (object != NULL &&
            (object->type == MONKEY_RETURN_VALUE ||
            object->type == MONKEY_ERROR))
            return object;
    }
    return object;
}

static monkey_object_t *
eval_program(program_t *program, environment_t *env)
{
    monkey_object_t *object = NULL;
    monkey_return_value_t *return_value_object;
    monkey_object_t *ret_value;
    for (size_t i = 0; i < program->nstatements; i++) {
        if (object)
            free_monkey_object(object);
        object = monkey_eval((node_t *) program->statements[i], env);
        if (object != NULL) {
            if (object->type == MONKEY_RETURN_VALUE) {
                return_value_object = (monkey_return_value_t *) object;
                ret_value = copy_monkey_object(return_value_object->value);
                free_monkey_object((monkey_object_t *) return_value_object);
                return ret_value;
            } else if (object->type == MONKEY_ERROR)
                return object;
        }
    }
    return object;
}

static monkey_object_t *
eval_statement(statement_t *statement, environment_t *env)
{
    expression_statement_t *exp_stmt;
    block_statement_t *block_stmt;
    return_statement_t *ret_stmt;
    letstatement_t *let_stmt;
    monkey_object_t *evaluated;
    switch (statement->statement_type)
    {
        case EXPRESSION_STATEMENT:
            exp_stmt = (expression_statement_t *) statement;
            return eval_expression(exp_stmt->expression, env);
        case BLOCK_STATEMENT:
            block_stmt = (block_statement_t *) statement;
            return eval_block_statement(block_stmt, env);
        case RETURN_STATEMENT:
            ret_stmt = (return_statement_t *) statement;
            evaluated = monkey_eval((node_t *) ret_stmt->return_value, env);
            if (is_error(evaluated))
                return evaluated;
            return (monkey_object_t *)create_monkey_return_value(evaluated);
        case LET_STATEMENT:
            let_stmt = (letstatement_t *) statement;
            evaluated = monkey_eval((node_t *) let_stmt->value, env);
            if (is_error(evaluated))
                return evaluated;
            if (evaluated == NULL)
                evaluated = (monkey_object_t *) create_monkey_null();
            env_put(env, strdup(let_stmt->name->value), evaluated);
        default:
            break;
    }
    return NULL;
}

monkey_object_t *
monkey_eval(node_t *node, environment_t *env)
{
    program_t *program;
    switch (node->type)
    {
        case STATEMENT:
            return eval_statement((statement_t *) node, env);
            break;
        case EXPRESSION:
            return eval_expression((expression_t *) node, env);
        case PROGRAM:
            program = (program_t *) node;
            return eval_program(program, env);
    }
}