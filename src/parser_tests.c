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
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "parser.h"
#include "lexer.h"
#include "test_utils.h"

static void
check_parser_errors(parser_t *parser)
{
    if (parser->errors == NULL)
        return;
    cm_list_node *errors_head = parser->errors->head;
    while (errors_head) {
        printf("parser error: %s\n", (char *) errors_head->data);
        errors_head = errors_head->next;
    }
    exit(1);
}

static _Bool
string_to_bool(const char *str)
{
    if (strncmp("true", str, 4) == 0)
        return true;
    else
        return false;
}

static void
test_integer_literal_value(expression_t *exp, long expected_value)
{
    test(exp->expression_type == INTEGER_EXPRESSION,
        "Expected expression to be of type %s, found %s\n",
        get_expression_type_name(INTEGER_EXPRESSION),
        get_expression_type_name(exp->expression_type));
    printf("The expression is of type INTEGER_EXPRESSION\n");
    integer_t *int_exp = (integer_t *) exp;
    test(int_exp->value == expected_value,
        "Expected value of integer expression to be %ld, found %ld\n",
        expected_value, int_exp->value);
    printf("Matched the value of the integer expression\n");
    char *literal = int_exp->expression.node.string(int_exp);
    char *expected_literal = long_to_string(expected_value);
    test(strcmp(literal, expected_literal) == 0,
        "Expected the token literal for the integer expression to be %s, found %s\n",
        literal, expected_literal);
    free(expected_literal);
    free(literal);
    printf("Matched the token literal for the integer expression\n");
}

static void
test_identifier(expression_t *exp, const char *expected_value)
{
    test(exp->expression_type == IDENTIFIER_EXPRESSION,
        "expected expression of type IDENTIFIER_EXPRESSION, found %s\n",
        get_expression_type_name(exp->expression_type));
    printf("The expression is an identifier\n");

    identifier_t *ident_exp = (identifier_t *) exp;
    test(strcmp(ident_exp->value, expected_value) == 0,
        "Expected identifier value to be %s, found %s\n",
        expected_value, ident_exp->value);
    printf("Matched identifier value\n");

    char *tok_literal = ident_exp->expression.node.token_literal(ident_exp);
    test(strcmp(tok_literal, expected_value) == 0,
        "Expected identifier token literal to be %s, found %s\n",
        expected_value, tok_literal);
    printf("Matched identifier token literal\n");
}

static void
test_boolean_literal(expression_t *exp, const char *expected_value)
{
    test(exp->expression_type == BOOLEAN_EXPRESSION,
        "Expected expression type BOOLEAN_EXPRESSION, found %s\n",
        get_expression_type_name(exp->expression_type));

    boolean_expression_t *bool_exp = (boolean_expression_t *) exp;
    test(bool_exp->value == string_to_bool(expected_value),
        "Expected to find boolean expression value %s, found %s\n",
        expected_value, bool_to_string(bool_exp->value));

    char *tok_literal = bool_exp->expression.node.token_literal(bool_exp);
    test(strcmp(tok_literal, expected_value) == 0,
        "Expected token literal for boolean expression: %s, found %s\n",
        expected_value, bool_to_string(bool_exp->value));
}

static void
test_literal_expression(expression_t *exp, const char *value)
{
    long expected_integer_value;
    switch (exp->expression_type) {
        case INTEGER_EXPRESSION:
            expected_integer_value = atol(value);
            test_integer_literal_value(exp, expected_integer_value);
            return;
        case IDENTIFIER_EXPRESSION:
            test_identifier(exp, value);
            return;
        case BOOLEAN_EXPRESSION:
            test_boolean_literal(exp, value);
            return;
        default:
            err(EXIT_FAILURE, "Unsupported expression type passed to test_literal_expression: %s",
                get_expression_type_name(exp->expression_type));
    }
}

static void
test_infix_expression(expression_t *exp, const char *operator, const char *left, const char *right)
{
    test(exp->expression_type == INFIX_EXPRESSION,
            "Expected to find expression of type INFIX_EXPRESSION, found %s\n",
            get_expression_type_name(exp->expression_type));
        printf("Found INFIX_EXPRESSION\n");

        infix_expression_t *infix_exp = (infix_expression_t *) exp;

        test_literal_expression(infix_exp->left, left);

        test(strcmp(infix_exp->operator, operator) == 0,
            "Expected infix expression operator to be %s, found %s\n",
            operator, infix_exp->operator);
        printf("matched infix expression operator\n");
        printf("Testing left expression of the infix expression\n");
        test_literal_expression(infix_exp->left, left);
        printf("Testing right expression of the infix expression\n");
        test_literal_expression(infix_exp->right, right);
        printf("Successfully tested parsing of infix expression\n");
}

static void
test_parser_errors(void)
{
    const char *input = "let x 5;\n"\
    "let = 10;\n"\
    "let 838383;\n";
    print_test_separator_line();
    printf("Testing parser errors\n");
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    test(program != NULL, "parse_program returned NULL\n");
    test(parser->errors != NULL, "expected to find 4 parsing errors, found 0\n");
    test(parser->errors->length == 4, "expected to find 4 errors, found %zu errors\n", parser->errors->length);
    cm_list_node *errors_head = parser->errors->head;
    token_type expected_tokens[] = {ASSIGN, IDENT, IDENT};
    token_type actual_tokens[] = {INT, ASSIGN, INT};
    int i = 0;
    while (errors_head) {
        char *expected_error = NULL;
        asprintf(&expected_error, "expected next token to be %s, got %s instead",
            get_token_name_from_type(expected_tokens[i]),
            get_token_name_from_type(actual_tokens[i]));
        test(strcmp((char *) errors_head->data, expected_error) == 0,
            "Failed test %d for parsing errors, expected error message to be \"%s\" found \"%s\"\n", i, expected_error,
            (char *)errors_head->data);
        free(expected_error);
        errors_head = errors_head->next;
        i++;
    }
    printf("Passed parsing errors test\n");
    program_free(program);
    parser_free(parser);
}


static void
_test_let_stmt(statement_t *stmt, const char *expected_identifier)
{
    char *tok_literal = stmt->node.token_literal(stmt);
    test(strcmp(tok_literal, "let") == 0,
        "Invalid token literal \"%s\" for LET STATEMENT\n",
        tok_literal);
    letstatement_t *let_stmt = (letstatement_t *) stmt;

    test(strcmp(let_stmt->name->value, expected_identifier) == 0,
        "Expected identifier value for let statement: \"%s\", found \"%s\"",
        expected_identifier, let_stmt->name->value);
    
    tok_literal = let_stmt->name->expression.node.token_literal(let_stmt->name);
    test(strcmp(tok_literal, expected_identifier) == 0,
        "Expected let statement identifier token literal: %s, found: %s",
        expected_identifier, tok_literal);
}

static void
test_let_stmt()
{
    lexer_t *lexer;
    parser_t *parser;
    program_t *program;
    statement_t *stmt;

    typedef struct {
        const char *input;
        const char *expected_identifier;
        const char *expected_value;
    } test_input;

    test_input tests[] = {
        {"let x = 5;\n", "x", "5"},
        {"let y = 10;\n", "y", "10"},
        {"let foobar = 838383;\n", "foobar", "838383"}
    };
    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    printf("Testing let statements\n");
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing let statement: %s\n", test.input);
        lexer = lexer_init(test.input);
        parser = parser_init(lexer);
        program = parse_program(parser);
        check_parser_errors(parser);
        test(program != NULL, "parse_program returned NULL\n");
        printf("program parsed successfully\n");
        test(program->nstatements == 1, \
            "program does not have 1 statements, found %zu ements", \
            program->nstatements);
        printf("matched the number of statements\n");

        stmt = program->statements[0];
        test(stmt->statement_type == LET_STATEMENT, \
            "Expected statement type to be LET_STATEMENT, found %s", \
            get_statement_type_name(stmt->statement_type));
        printf("matched let statement type\n");
        _test_let_stmt(stmt, test.expected_identifier);
        printf("matched identifier\n");
        letstatement_t *let_stmt = (letstatement_t *) stmt;
        test_literal_expression(let_stmt->value, test.expected_value);
        printf("Tested let statement value\n");
        program_free(program);
        parser_free(parser);
    }
}

static void
test_identifier_expression()
{
    const char *input = "foobar;\n";
    print_test_separator_line();
    printf("Testing identifier expression\n");
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    check_parser_errors(parser);
    test(program != NULL, "parse_program failed\n");
    printf("program parsed successfully\n");

    test(program->nstatements == 1, "expected program to have 1statements, found %zu\n",
        program->nstatements);
    printf("parsed correct number of statements\n");

    test(program->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "expected node of type expression statement, found %s",
        get_statement_type_name(program->statements[0]->statement_type));
    printf("Found only 1 statement\n");

    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    test(exp_stmt->expression->expression_type == IDENTIFIER_EXPRESSION,
        "expected expression of type expression, found %s",
        get_expression_type_name(exp_stmt->expression->expression_type));
    printf("Found an identifier expression\n");

    identifier_t *ident = (identifier_t *) exp_stmt->expression;
    test(strcmp(ident->value, "foobar") == 0,
        "expected the identifier value to be foobar, found %s", ident->value);
    printf("Matched the value of the identifier\n");

    char *ident_token_literal = ident->expression.node.token_literal(ident);
    test(strcmp(ident_token_literal, "foobar") == 0,
        "expected identifier token literal to be foobar, found %s",
        ident_token_literal);
    printf("Matched the value of the token literal for the identifier\n");
    printf("Identifier parsing test passed\n");
    program_free(program);
    parser_free(parser);
}

static void
test_parse_infix_expression()
{
    typedef struct test_input {
        const char *input;
        const char *operator;
        const char *left;
        const char *right;
    } test_input;

    test_input tests[] = {
        {"5 + 5;", "+", "5", "5"},
        {"5 - 5;", "-", "5", "5"},
        {"5 * 5;", "*", "5", "5"},
        {"5 / 5;", "/", "5", "5"},
        {"5 > 5;", ">", "5", "5"},
        {"5 < 5;", "<", "5", "5"},
        {"5 == 5;", "==", "5", "5"},
        {"5 != 5;", "!=", "5", "5"},
        {"foobar + barfoo;", "+", "foobar", "barfoo"},
        {"foobar - barfoo;", "-", "foobar", "barfoo"},
        {"foobar / barfoo;", "/", "foobar", "barfoo"},
        {"foobar * barfoo;", "*", "foobar", "barfoo"},
        {"foobar > barfoo;", ">", "foobar", "barfoo"},
        {"foobar < barfoo;", "<", "foobar", "barfoo"},
        {"foobar == barfoo;", "==", "foobar", "barfoo"},
        {"foobar != barfoo;", "!=", "foobar", "barfoo"},
        {"true == true", "==", "true", "true"},
        {"true != false", "!=", "true", "false"},
        {"false == false", "==", "false", "false"},
        {"true && false", "&&", "true", "false"},
        {"true || false", "||", "true", "false"},
        {"10 % 3", "%", "10", "3"}
    };
    print_test_separator_line();
    printf("Testing infix expressions\n");
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing expression: %s\n", test.input);
        lexer_t *lexer = lexer_init(test.input);
        parser_t *parser = parser_init(lexer);
        program_t *program = parse_program(parser);
        check_parser_errors(parser);
        test(program->nstatements == 1,
            "Expected to find 1 statement, found %zu\n",
            program->nstatements);
        printf("Found correct number of statements\n");

        test(program->statements[0]->statement_type == EXPRESSION_STATEMENT,
            "Expected to find EXPRESSION_STATEMENT, found %s\n",
            get_statement_type_name(program->statements[0]->statement_type));
        printf("Found correct statement type\n");

        expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
        test_infix_expression(exp_stmt->expression, test.operator, test.left, test.right);
        program_free(program);
        parser_free(parser);
    }
}

static void
test_operator_precedence_parsing()
{
    print_test_separator_line();
    printf("Testing operator precedence parsing\n");
    typedef struct {
        const char *input;
        const char *string;
    } test_input;

    test_input tests[] = {
        {"-a * b", "((-a) * b)"},
        {"!-a", "(!(-a))"},
        {"a + b + c", "((a + b) + c)"},
        {"a + b - c", "((a + b) - c)"},
        {"a * b * c", "((a * b) * c)"},
        {"a * b / c", "((a * b) / c)"},
        {"a + b / c", "(a + (b / c))"},
        {"a + b * c + d / e - f", "(((a + (b * c)) + (d / e)) - f)"},
        {"3 + 4; -5 * 5", "(3 + 4) ((-5) * 5)"},
        {"5 > 4 == 3 < 4", "((5 > 4) == (3 < 4))"},
        {"5 < 4 != 3 > 4", "((5 < 4) != (3 > 4))"},
        {"3 + 4 * 5 == 3 * 1 + 4 * 5", "((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))"},
        {"true", "true"},
        {"false", "false"},
        {"3 > 5 == false", "((3 > 5) == false)"},
        {"3 < 5 == true", "((3 < 5) == true)"},
        {"1 + (2 + 3) + 4", "((1 + (2 + 3)) + 4)"},
        {"(5 + 5) * 2", "((5 + 5) * 2)"},
        {"2 / (5 + 5)", "(2 / (5 + 5))"},
        {"-(5 + 5)", "(-(5 + 5))"},
        {"!(true == true)", "(!(true == true))"},
        {"a + add(b * c) + d", "((a + add((b * c))) + d)"},
        {"add(a, b, 1, 2 * 3, 4 + 5, add(6, 7 *  8))", "add(a, b, 1, (2 * 3), (4 + 5), add(6, (7 * 8)))"},
        {"add(a + b + c * d / f + g)", "add((((a + b) + ((c * d) / f)) + g))"},
        {"a * [1, 2, 3, 4][b * c] * d", "((a * ([1, 2, 3, 4][(b * c)])) * d)"},
        {"add(a * b[2], b[1], 2 * [1, 2][1])", "add((a * (b[2])), (b[1]), (2 * ([1, 2][1])))"},
        {"5 > 4 && 3 > 2", "((5 > 4) && (3 > 2))"},
        {"4 < 5 || 3 > 2", "((4 < 5) || (3 > 2))"}
    };

    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing expression: %s\n", test.input);
        lexer_t *lexer = lexer_init(test.input);
        parser_t *parser = parser_init(lexer);
        program_t *program = parse_program(parser);
        check_parser_errors(parser);
        char *actual_string = program->node.string(program);
        test(strcmp(test.string, actual_string) == 0,
            "Expected program string: \"%s\", found: \"%s\"\n",
            test.string, actual_string);
        free(actual_string);
        program_free(program);
        parser_free(parser);
    }
}

static void
test_parse_prefix_expression()
{
    typedef struct test_input {
        const char *input;
        const char *operator;
        const char *value;
    } test_input;

    test_input tests[] = {
        {"!5", "!", "5"},
        {"-15", "-", "15"},
        {"!foobar", "!", "foobar"},
        {"-foobar", "-", "foobar"},
        {"!true", "!", "true"},
        {"!false", "!", "false"}
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests)/sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing prefix expression: %s\n", test.input);
        lexer_t *lexer = lexer_init(test.input);
        parser_t *parser = parser_init(lexer);
        program_t *program = parse_program(parser);
        check_parser_errors(parser);
        test(program != NULL, "failed to parse the program\n");
        printf("Program parsed successfully\n");

        test(program->nstatements == 1,
            "Expected program to have 1 statement, found %zu\n",
            program->nstatements);
        printf("Found correct number of statements in the program\n");

        test(program->statements[0]->statement_type == EXPRESSION_STATEMENT,
            "Expected to find a statement of type EXPRESSION_STATEMENT, found %s\n",
            get_statement_type_name(program->statements[0]->statement_type));
        printf("Successfully parsed expression statement\n");

        expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
        test(exp_stmt->expression->expression_type == PREFIX_EXPRESSION,
            "Expected to find PREFIX_EXPRESSION, found %s\n",
            get_expression_type_name(exp_stmt->expression->expression_type));
        printf("Found PREFIX_EXPRESSION\n");

        prefix_expression_t *prefix_exp = (prefix_expression_t *) exp_stmt->expression;
        test(strcmp(prefix_exp->operator, test.operator) == 0,
            "Expected operator to be %s, found %s\n", test.operator, prefix_exp->operator);
        printf("Passed prefix operator test\n");

        test_literal_expression(prefix_exp->right, test.value);
        printf("Found correct operand value for the prefix test\n");
        program_free(program);
        parser_free(parser);
    }
    printf("Prefix expression parsing tests passed\n");
}

static void
test_integer_literal_expression()
{
    const char *input = "5;\n";
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    check_parser_errors(parser);
    test(program != NULL, "parse_program failed\n");
    printf("program parsed successfully\n");

    test(program->nstatements == 1, "expected program to have 1statements, found %zu\n",
        program->nstatements);
    printf("parsed correct number of statements\n");

    test(program->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "expected node of type expression statement, found %s",
        get_statement_type_name(program->statements[0]->statement_type));
    printf("Found only 1 statement\n");

    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    test(exp_stmt->expression->expression_type == INTEGER_EXPRESSION,
        "expected expression of type expression, found %s",
        get_expression_type_name(exp_stmt->expression->expression_type));
    printf("Found an integer expression\n");
    test_integer_literal_value(exp_stmt->expression, 5);
    printf("integer expression parsing test passed\n");
    program_free(program);
    parser_free(parser);

}

static void
test_return_statement()
{
    typedef struct {
        const char *input;
        const char *expected_value;
    } test_input;

    test_input tests[] = {
        {"return 5;", "5"},
        {"return true;", "true"},
        {"return foobar;", "foobar"}
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing return statement: %s\n", test.input);
        lexer_t *lexer = lexer_init(test.input);
        parser_t *parser = parser_init(lexer);
        program_t *program = parse_program(parser);
        check_parser_errors(parser);
        test(program != NULL, "parse_program failed\n");

        printf("program parsed successfully\n");
        test(program->nstatements == 1, "expected program to have 1 statements, found %zu\n",
            program->nstatements);
        printf("parsed correct number of statements\n");

        statement_t *stmt = program->statements[0];
        node_t stmt_node = stmt->node;
        char *tok_literal = stmt_node.token_literal(stmt);
        test(strcmp(tok_literal, "return") == 0, "expected token literal to be \"return\"," \
            " found \"%s\"\n", tok_literal);
        printf("matched token literal for return statement\n");
        test(stmt->statement_type == RETURN_STATEMENT, "expected statement type to be %s," \
            "found %s\n", get_statement_type_name(RETURN_STATEMENT),
            get_statement_type_name(stmt->statement_type));
        printf("matched statement type for return\n");

        return_statement_t *ret_stmt = (return_statement_t *) stmt;
        test_literal_expression(ret_stmt->return_value, test.expected_value);
        program_free(program);
        parser_free(parser);
    }
}

static void
test_string()
{
    const char *input = "let myvar = someVar;";
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    check_parser_errors(parser);
    char *program_string = program->node.string(program);
    test(strcmp(input, program_string) == 0, "Expected program string to be \"%s\"," \
        "found \"%s\"\n", input, program_string);
    program_free(program);
    parser_free(parser);
    free(program_string);
}

static void
test_boolean_expression()
{
    typedef struct {
        const char *input;
        const char *expected_value;
    } test_input;

    test_input tests[] = {
        {"true;", "true"},
        {"false;", "false"}
    };

    print_test_separator_line();
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Parsing boolean expression: %s\n", test.input);
        lexer_t *lexer = lexer_init(test.input);
        parser_t *parser = parser_init(lexer);
        program_t *program = parse_program(parser);
        check_parser_errors(parser);

        test(program != NULL, "Failed to parse boolean expression\n");
        test(program->nstatements == 1,
            "Expected 1 statement in program, found %zu\n", program->nstatements);
        printf("Program parsed successfully\n");

        test(program->statements[0]->statement_type == EXPRESSION_STATEMENT,
            "Expected EXPRESSION_STATEMENT, found %s\n",
            get_statement_type_name(program->statements[0]->statement_type));
        printf("Found EXPRESSION_STATEMENT\n");

        expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
        test_literal_expression(exp_stmt->expression, test.expected_value);
        program_free(program);
        parser_free(parser);
    }
}

static void
test_ifelse_expression(void)
{
    const char *input = "if (x < y) { x } else { y }";
    print_test_separator_line();
    printf("Testing if-else expression: %s\n", input);
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    test(program != NULL, "Failed to parse program\n");

    check_parser_errors(parser);
    test(program->nstatements == 1,
        "Expected 1 statement in program, found %zu\n", program->nstatements);

    test(program->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "Expected EXPRESSION_STATEMENT, found %s\n",
        get_statement_type_name(program->statements[0]->statement_type));

    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    expression_t *exp = exp_stmt->expression;

    test(exp->expression_type == IF_EXPRESSION,
        "Expected IF_EXPRESSION, found %s\n", get_expression_type_name(exp->expression_type));

    if_expression_t *if_exp = (if_expression_t *) exp;
    test_infix_expression(if_exp->condition, "<", "x", "y");

    test(if_exp->consequence->nstatements == 1,
        "Expected 1 statement in consequence, found %zu\n",
        if_exp->consequence->nstatements);

    test(if_exp->consequence->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "Expected statement of type EXPRESSION_STATEMENT in consequence, found %s\n",
        get_statement_type_name(if_exp->consequence->statements[0]->statement_type));

    expression_statement_t *conseq_stmt = (expression_statement_t *) if_exp->consequence->statements[0];
    test_identifier(conseq_stmt->expression, "x");

    test(if_exp->alternative->nstatements == 1,
        "Expected 1 statement in alternative of if expression, found %zu\n",
        if_exp->alternative->nstatements);

    test(if_exp->alternative->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "Expected statement of type EXPRESSION_STATEMENT in alternative of if expression, found %s\n",
        get_statement_type_name(if_exp->alternative->statements[0]->statement_type));

    expression_statement_t *alternative_exp_stmt = (expression_statement_t *) if_exp->alternative->statements[0];
    test_identifier(alternative_exp_stmt->expression, "y");

    program_free(program);
    parser_free(parser);
    printf("if-else expression test passed\n");
}


static void
test_if_expression(void)
{
    const char *input = "if (x < y) { x }";
    print_test_separator_line();
    printf("Testing if expression: %s\n", input);
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    test(program != NULL, "Failed to parse program\n");

    check_parser_errors(parser);
    test(program->nstatements == 1,
        "Expected 1 statement in program, found %zu\n", program->nstatements);

    test(program->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "Expected EXPRESSION_STATEMENT, found %s\n",
        get_statement_type_name(program->statements[0]->statement_type));

    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    expression_t *exp = exp_stmt->expression;

    test(exp->expression_type == IF_EXPRESSION,
        "Expected IF_EXPRESSION, found %s\n", get_expression_type_name(exp->expression_type));

    if_expression_t *if_exp = (if_expression_t *) exp;
    test_infix_expression(if_exp->condition, "<", "x", "y");

    test(if_exp->consequence->nstatements == 1,
        "Expected 1 statement in consequence, found %zu\n",
        if_exp->consequence->nstatements);

    test(if_exp->consequence->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "Expected statement of type EXPRESSION_STATEMENT in consequence, found %s\n",
        get_statement_type_name(if_exp->consequence->statements[0]->statement_type));

    expression_statement_t *conseq_stmt = (expression_statement_t *) if_exp->consequence->statements[0];
    test_identifier(conseq_stmt->expression, "x");

    test(if_exp->alternative == NULL,
        "Expected alternative of if expression to be NULL\n");

    program_free(program);
    parser_free(parser);
    printf("if expression test passed\n");
}

static void
test_function_literal(void)
{
    const char *input = "fn(x, y) { x + y; }";
    print_test_separator_line();
    printf("Testing function literal: %s\n", input);
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    test(program != NULL, "Failed to parser program\n");
    check_parser_errors(parser);

    test(program->nstatements == 1, "Expected 1 statement in program, found %zu\n",
        program->nstatements);

    test(program->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "Expected statement of type EXPRESSION_STATEMENT, found %s\n",
        get_statement_type_name(program->statements[0]->statement_type));

    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    test(exp_stmt->expression->expression_type == FUNCTION_LITERAL,
        "Expected expression of type FUNCTION_LITERAL, found %s\n",
        get_expression_type_name(exp_stmt->expression->expression_type));

    function_literal_t *function = (function_literal_t *) exp_stmt->expression;
    test(function->parameters->length == 2, "Expected 2 parameters in function, found %zu\n",
        function->parameters->length);

    test_literal_expression((expression_t *) function->parameters->head->data, "x");
    test_literal_expression((expression_t *) function->parameters->head->next->data, "y");

    test(function->body->nstatements == 1, "Expected 1 statement in function body, found %zu\n",
        function->body->nstatements);

    test(function->body->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "Expected statement of type EXPRESSION_STATEMENT in function body, found %s\n",
        get_statement_type_name(function->body->statements[0]->statement_type));

    expression_statement_t *body_exp_statement = (expression_statement_t *) function->body->statements[0];
    test_infix_expression(body_exp_statement->expression, "+", "x", "y");
    printf("Function literal parsing test passed\n");
    program_free(program);
    parser_free(parser);
}

static void
test_function_parameter_parsing(void)
{
    typedef struct {
        const char *input;
        size_t nparams;
        const char *expected_params[3];
    } test_input;

    test_input tests[] = {
        {"fn () {};", 0, {NULL, NULL, NULL}},
        {"fn (x) {};", 1, {"x", NULL, NULL}},
        {"fn (x, y, z) {};", 3, {"x", "y", "z"}}
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);
    print_test_separator_line();

    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing function parameter parsing for: %s\n", test.input);
        lexer_t *lexer = lexer_init(test.input);
        parser_t *parser = parser_init(lexer);
        program_t *program = parse_program(parser);
        test(program != NULL, "Program parsing failed\n");
        check_parser_errors(parser);

        expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
        function_literal_t *function = (function_literal_t *) exp_stmt->expression;
        test(function->parameters->length == test.nparams,
            "Expected %zu parameters, found %zu\n", test.nparams, function->parameters->length);

        cm_list_node *list_node = function->parameters->head;
        for (size_t i = 0; i < test.nparams; i++) {
            identifier_t *param = (identifier_t *) list_node->data;
            test_literal_expression((expression_t *) param, test.expected_params[i]);
            list_node = list_node->next;
        }

        program_free(program);
        parser_free(parser);
    }
}

static void
test_call_expression_parsing(void)
{
    const char *input = "add(1, 2 * 3, 4 + 5);";
    print_test_separator_line();
    printf("Testing call expression parsing\n");
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    test(program != NULL, "Failed to parse program\n");
    check_parser_errors(parser);

    test(program->nstatements == 1, "Expected 1 statement in program, found %zu\n",
        program->nstatements);

    test(program->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "Expected statement of type EXPRESSION_STATEMENT, found %s\n",
        get_statement_type_name(program->statements[0]->statement_type));

    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    test(exp_stmt->expression->expression_type == CALL_EXPRESSION,
        "Expected expression of type CALL_EXPRESSION, found %s\n",
        get_expression_type_name(exp_stmt->expression->expression_type));

    test(exp_stmt->expression->expression_type == CALL_EXPRESSION, "Expected CALL_EXPRESSION found %s\n",
        get_expression_type_name(exp_stmt->expression->expression_type));

    call_expression_t *call_exp = (call_expression_t *) exp_stmt->expression;
    test(call_exp->function->expression_type == IDENTIFIER_EXPRESSION,
        "Expected function to be IDENTIFIER_EXPRESSION, found %s\n",
        get_expression_type_name(call_exp->function->expression_type));
    test_identifier(call_exp->function, "add");
    test(call_exp->arguments->length == 3, "Expected 3 arguments, found %zu\n",
        call_exp->arguments->length);

    test_literal_expression((expression_t *) call_exp->arguments->head->data, "1");
    test_infix_expression((expression_t *) call_exp->arguments->head->next->data,
        "*", "2", "3");
    test_infix_expression((expression_t *) call_exp->arguments->head->next->next->data,
        "+", "4", "5");
    program_free(program);
    parser_free(parser);
    printf("Call expression parsing test passed\n");
}

static void
test_call_expression_argument_parsing(void)
{
    print_test_separator_line();
    typedef struct {
        const char *input;
        const char *expected_ident;
        size_t nargs;
        const char *expected_args[3];
    } test_input;

    test_input tests[] = {
        {"add();", "add", 0, {NULL, NULL, NULL}},
        {"add(1);", "add", 1, {"1", NULL, NULL}},
        {"add(1, 2 * 3, 4 + 5);", "add", 3, {"1", "(2 * 3)", "(4 + 5)"}}
    };
    size_t ntests = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
        printf("Testing call expression argument parsing for %s\n", test.input);
        lexer_t *lexer = lexer_init(test.input);
        parser_t *parser = parser_init(lexer);
        program_t *program = parse_program(parser);
        test(program != NULL, "Failed to parse program\n");
        check_parser_errors(parser);

        expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
        test(exp_stmt->expression->expression_type == CALL_EXPRESSION,
            "Expected CALL_EXPRESSION, found %s\n", get_expression_type_name(exp_stmt->expression->expression_type));

        call_expression_t *call_exp = (call_expression_t *) exp_stmt->expression;
        test_identifier(call_exp->function, test.expected_ident);
        test(call_exp->arguments->length == test.nargs,
            "Expected %zu arguments in call expression, found %zu\n",
            test.nargs, call_exp->arguments->length);

        cm_list_node *list_node = call_exp->arguments->head;
        for (size_t j = 0; j < test.nargs; j++) {
            expression_t *arg = (expression_t *) list_node->data;
            char *arg_string = arg->node.string(arg);
            test(strcmp(arg_string, test.expected_args[j]) == 0,
                "Expected argument %zu to be %s, found %s\n",
                j, test.expected_args[j], arg_string);
            free(arg_string);
            list_node = list_node->next;
        }
        program_free(program);
        parser_free(parser);
    }
    printf("Call expression argument tests passed\n");
}

static void
test_string_literal(void)
{
    const char *input = "\"hello, world!\"";
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    print_test_separator_line();
    printf("Test string expression parsing\n");
    test(program->nstatements == 1, "Expected 1 statement, found %zu\n",
        program->nstatements);
    test(program->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "Expected EXPRESSION_STATEMENT, found %s\n",
        get_statement_type_name(program->statements[0]->statement_type));
    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    test(exp_stmt->expression->expression_type == STRING_EXPRESSION,
        "Expected expression of type STRING_EXPRESSION, found %s\n",
        get_expression_type_name(exp_stmt->expression->expression_type));
    string_t *string = (string_t *) exp_stmt->expression;
    test(strcmp(string->value, "hello, world!") == 0,
        "Expected string literal \"hello, world!\", found \"%s\"\n", string->value);
    program_free(program);
    parser_free(parser);
}

static void
test_parse_array_literal(void)
{
    const char *input = "[1, 2 * 2,  3 + 3]";
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    check_parser_errors(parser);
    print_test_separator_line();
    printf("Testing array literal expression\n");
    test(program->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "Expected EXPRESSION_STATEMENT, found %s\n",
        get_statement_type_name(program->statements[0]->statement_type));
    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    test(exp_stmt->expression->expression_type == ARRAY_LITERAL,
        "Expected ARRAY_LITERAL, found %s\n", get_expression_type_name(exp_stmt->expression->expression_type));
    array_literal_t *array = (array_literal_t *) exp_stmt->expression;
    test(array->elements->length == 3, "Expected 3 elements in array, found %zu\n",
        array->elements->length);
    test_integer_literal_value((expression_t *) array->elements->array[0], 1);
    test_infix_expression((expression_t *) array->elements->array[1], "*", "2", "2");
    test_infix_expression((expression_t *) array->elements->array[2], "+", "3", "3");
    program_free(program);
    parser_free(parser);
}

static void
test_parse_index_expression(void)
{
    const char *input = "my_array[1 + 1]";
    print_test_separator_line();
    printf("Testing index expression parsing\n");
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    check_parser_errors(parser);
    test(program->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "Expected EXPRESSION_STATEMENT, got %s\n",
        get_statement_type_name(program->statements[0]->statement_type));
    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    test(exp_stmt->expression->expression_type == INDEX_EXPRESSION,
        "Expected EXPRESSION_STATEMENT, got %s\n", get_expression_type_name(exp_stmt->expression->expression_type));
    index_expression_t *index_exp = (index_expression_t *) exp_stmt->expression;
    test_identifier(index_exp->left, "my_array");
    test_infix_expression(index_exp->index, "+", "1", "1");
    program_free(program);
    parser_free(parser);
    printf("Index expression parsing test passed\n");
}

static void
test_parse_hash_literals(void)
{
    const char *input = "{\"one\": 1, \"two\": 2, \"three\": 3}";
    cm_hash_table *expected = cm_hash_table_init(string_hash_function,
        string_equals, NULL, NULL);
    int one = 1;
    int two = 2;
    int three = 3;
    cm_hash_table_put(expected, "one", &one);
    cm_hash_table_put(expected, "two", &two);
    cm_hash_table_put(expected, "three", &three);
    print_test_separator_line();
    printf("Testing hash literal parsing: %s\n", input);
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    check_parser_errors(parser);
    test(program->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "Expected EXPRESSION_STATEMENT, got %s\n",
        get_statement_type_name(program->statements[0]->statement_type));
    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    test(exp_stmt->expression->expression_type == HASH_LITERAL,
        "Expected HASH_LITERAL, got %s\n",
        get_expression_type_name(exp_stmt->expression->expression_type));
    hash_literal_t *hash_exp = (hash_literal_t *) exp_stmt->expression;
    test(hash_exp->pairs->nkeys == 3,
        "Expected 3 entries in the hash pairs, got %zu\n",
        hash_exp->pairs->nkeys);
    for (size_t i = 0; i < hash_exp->pairs->used_slots->length; i++) {
        size_t *index = (size_t *) hash_exp->pairs->used_slots->array[i];
        cm_list *entry_list = hash_exp->pairs->table[*index];
        test(entry_list != NULL, "No entry found at index %zu in the pairs table\n", *index);
        cm_list_node *node = entry_list->head;
        while (node != NULL) {
            cm_hash_entry *entry = (cm_hash_entry *) node->data;
            expression_t *key = (expression_t *) entry->key;
            int *expected_value = cm_hash_table_get(expected, ((string_t *)key)->value);
            test(expected_value != NULL, "unknown key %s found in pairs\n", ((string_t *) key)->value);
            integer_t *actual_value = (integer_t *) entry->value;
            test_integer_literal_value((expression_t *) actual_value, *expected_value);
            node = node->next;
        }
    }
    program_free(program);
    parser_free(parser);
    cm_hash_table_free(expected);
}

static void
test_parsing_empty_hash_literal(void)
{
    const char *input = "{}";
    print_test_separator_line();
    printf("Testing parsing of empty hash literal\n");
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    check_parser_errors(parser);
    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    test(exp_stmt->expression->expression_type == HASH_LITERAL,
        "Expected HASH_LITERAL expression, found %s\n",
        get_expression_type_name(exp_stmt->expression->expression_type));
    hash_literal_t *hash_exp = (hash_literal_t *) exp_stmt->expression;
    test(hash_exp->pairs->nkeys == 0,
        "Expected 0 entries in hash literal, found %zu\n",
        hash_exp->pairs->nkeys);
    program_free(program);
    parser_free(parser);
}

static void
test_parsing_hash_literal_bool_keys(void)
{
    const char *input = "{true: 1, false: 2}";
    print_test_separator_line();
    printf("Testing parsing of hash literals with boolean keys\n");
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    check_parser_errors(parser);
    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    test(exp_stmt->expression->expression_type == HASH_LITERAL,
        "Expected HASH_LITERAL expression, found %s\n",
        get_expression_type_name(exp_stmt->expression->expression_type));
    hash_literal_t *hash_exp = (hash_literal_t *) exp_stmt->expression;
    for(size_t i = 0; i < hash_exp->pairs->nkeys; i++) {
        size_t *index = (size_t *) hash_exp->pairs->used_slots->array[i];
        cm_hash_entry *entry = (cm_hash_entry *) hash_exp->pairs->table[*index]->head->data;
        expression_t *key_exp = (expression_t *) entry->key;
        test(key_exp->expression_type == BOOLEAN_EXPRESSION,
            "Expected BOOLEAN_EXPRESSION as key, found %s\n",
            get_expression_type_name(key_exp->expression_type));
        expression_t *value_exp = (expression_t *) entry->value;
        test(value_exp->expression_type == INTEGER_EXPRESSION,
            "Expected INTEGER_EXPRESSION as value, found %s\n",
            get_expression_type_name(value_exp->expression_type));
        boolean_expression_t *bool_key = (boolean_expression_t *) key_exp;
        integer_t *int_value = (integer_t *) value_exp;
        if (bool_key->value) {
            test(int_value->value == 1,
                "Expected value for key true to be 1, found %ld\n",
                int_value->value);
        } else {
            test(int_value->value == 2,
                "Expected value for key false to be 2, found %ld\n",
                int_value->value);
        }
    }
    parser_free(parser);
    program_free(program);
}

static void
test_parsing_hash_literal_with_expression_values(void)
{
    typedef struct {
        const char *operator;
        const char *left;
        const char *right;
    } expected_value;

    const char *input = "{\"one\": 0 + 1, \"two\": 10 - 8, \"three\": 15 / 5}";
    print_test_separator_line();
    printf("Testing parsing of hash literal with expressions in values\n");
    cm_hash_table *expected = cm_hash_table_init(string_hash_function,
        string_equals, NULL, free_expression);
    cm_hash_table_put(expected, "one", &((expected_value ) {"+", "0", "1"}));
    cm_hash_table_put(expected, "two", &((expected_value) {"-", "10", "8"}));
    cm_hash_table_put(expected, "three", &((expected_value) {"/", "15", "5"}));
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    check_parser_errors(parser);
    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    test(exp_stmt->expression->expression_type == HASH_LITERAL,
        "Expected HASH_LITERAL expression, got %s\n",
        get_expression_type_name(exp_stmt->expression->expression_type));
    hash_literal_t *hash_exp = (hash_literal_t *) exp_stmt->expression;
    test(hash_exp->pairs->nkeys == 3,
        "Expected 3 entries in hash literal, found %zu\n",
        hash_exp->pairs->nkeys);
    for (size_t i = 0; i < hash_exp->pairs->used_slots->length; i++) {
        size_t *index = (size_t *) hash_exp->pairs->used_slots->array[i];
        cm_hash_entry *entry = (cm_hash_entry *) hash_exp->pairs->table[*index]->head->data;
        expression_t *key = (expression_t *) entry->key;
        test(key->expression_type == STRING_EXPRESSION,
            "Expected STRING_EXPRESSION as key, found %s\n",
            get_expression_type_name(key->expression_type));
        string_t *string_exp = (string_t *) key;
        expression_t *value_exp = (expression_t *) entry->value;
        expected_value *exp_value = (expected_value *) cm_hash_table_get(expected, string_exp->value);
        test(exp_value != NULL, "Found an invalid key: %s in the pairs\n", string_exp->value);
        test_infix_expression(value_exp, exp_value->operator, exp_value->left, exp_value->right);
    }
    program_free(program);
    parser_free(parser);
    cm_hash_table_free(expected);
}

static void
test_parsing_hash_literal_with_integer_keys(void)
{
    cm_hash_table *expected = cm_hash_table_init(string_hash_function,
        string_equals, NULL, NULL);
    cm_hash_table_put(expected, "1", ((long []) {1}));
    cm_hash_table_put(expected, "2", ((long []) {2}));
    cm_hash_table_put(expected, "3", ((long []) {3}));
    const char *input = "{1: 1, 2: 2, 3:3}";
    print_test_separator_line();
    printf("Testing hash literal parsing with integer keys\n");
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    check_parser_errors(parser);
    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    test(exp_stmt->expression->expression_type == HASH_LITERAL,
        "Expected a HASH_LITERAL expression, got %s\n",
        get_expression_type_name(exp_stmt->expression->expression_type));
    hash_literal_t *hash_exp = (hash_literal_t *) exp_stmt->expression;
    test(hash_exp->pairs->nkeys == 3,
        "Expected 3 entries in pairs, found %zu\n", hash_exp->pairs->nkeys);
    for (size_t i = 0; i < hash_exp->pairs->used_slots->length; i++) {
        size_t *index = (size_t *) hash_exp->pairs->used_slots->array[i];
        cm_hash_entry *entry = (cm_hash_entry *) hash_exp->pairs->table[*index]->head->data;
        test(entry != NULL, "No entry found at slot %zu\n", *index);
        expression_t *key_exp = (expression_t *) entry->key;
        char *string_key = key_exp->node.string(key_exp);
        long *expected_value = cm_hash_table_get(expected, string_key);
        test_integer_literal_value(key_exp, expected_value[0]);
        free(string_key);
    }
    parser_free(parser);
    program_free(program);
    cm_hash_table_free(expected);
}

static void
test_function_literal_with_name(void)
{
    const char *input = "let myfn = fn() {};";
    print_test_separator_line();
    printf("Testing function literal with name: %s\n", input);
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    check_parser_errors(parser);
    test(program->nstatements == 1, "Expected 1 statement in program, found %zu\n", program->nstatements);
    test(program->statements[0]->statement_type == LET_STATEMENT, "Expected LET_STATEMENT, got %s\n",
        get_statement_type_name(program->statements[0]->statement_type));
    letstatement_t *letstmt = (letstatement_t *) program->statements[0];
    test(letstmt->value->expression_type == FUNCTION_LITERAL,
        "Expected let statement value to be function literal, found %s\n",
        get_expression_type_name(letstmt->value->expression_type));
    function_literal_t *fn_literal = (function_literal_t *) letstmt->value;
    test(strcmp(fn_literal->name, "myfn") == 0,
    "Expected function name myfn, found %s\n", fn_literal->name);
    parser_free(parser);
    program_free(program);

}

static void
test_parsing_while_expression(void)
{
    const char *input = "while (x > 2) {\n"\
        "   let x = x - 1;\n"\
        "   x;\n"\
        "}";
    print_test_separator_line();
    printf("Testing while expression parsing for: %s\n", input);
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    check_parser_errors(parser);

    test(program->nstatements == 1, "Expected 1 statement in program, found %zu\n",
        program->nstatements);
    test(program->statements[0]->statement_type == EXPRESSION_STATEMENT,
        "Expected EXPRESSION_STATEMENT, got %s\n",
        get_statement_type_name(program->statements[0]->statement_type));
    expression_statement_t *exp_stmt = (expression_statement_t *) program->statements[0];
    test(exp_stmt->expression->expression_type == WHILE_EXPRESSION,
        "Expected a WHILE_EXPRESSION, got %s\n",
        get_expression_type_name(exp_stmt->expression->expression_type));
    while_expression_t *while_exp = (while_expression_t *) exp_stmt->expression;
    test_infix_expression(while_exp->condition, ">", "x", "2");
    test(while_exp->body->nstatements == 2,
        "Expected 1 statement in while expression body, got %zu\n",
        while_exp->body->nstatements);
    test(while_exp->body->statements[0]->statement_type == LET_STATEMENT,
        "Expected LET_STATEMENT, got %s\n",
        get_statement_type_name(while_exp->body->statements[0]->statement_type));
    test(while_exp->body->statements[1]->statement_type == EXPRESSION_STATEMENT,
        "Expected EXPRESSION_STATEMENT, got %s\n",
        get_statement_type_name(while_exp->body->statements[0]->statement_type));
    program_free(program);
    parser_free(parser);
}

int
main(int argc, char **argv)
{
    test_let_stmt();
    test_return_statement();
    test_identifier_expression();
    test_integer_literal_expression();
    test_parse_prefix_expression();
    test_parse_infix_expression();
    test_operator_precedence_parsing();
    test_string();
    test_boolean_expression();
    test_if_expression();
    test_ifelse_expression();
    test_function_literal();
    test_function_parameter_parsing();
    test_call_expression_parsing();
    test_call_expression_argument_parsing();
    test_string_literal();
    test_parse_array_literal();
    test_parse_index_expression();
    test_parse_hash_literals();
    test_parsing_empty_hash_literal();
    test_parsing_hash_literal_with_expression_values();
    test_parsing_hash_literal_with_integer_keys();
    test_parsing_hash_literal_bool_keys();
    test_parsing_while_expression();
    test_function_literal_with_name();
    printf("All tests passed\n");

}
