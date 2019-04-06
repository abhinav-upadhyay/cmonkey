#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "parser.h"
#include "lexer.h"
#include "test_utils.h"

static void
print_test_separator_line(void)
{
    for (int i = 0; i < 100; i++)
        printf("-");
    printf("\n");
}

static void
check_parser_errors(parser_t *parser)
{
    if (parser->errors == NULL)
        return;
    cm_list_node *errors_head = parser->errors->head;
    while (errors_head) {
        printf("parser error: %s\n", errors_head->data);
        errors_head = errors_head->next;
    }
    exit(1);
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
        {"5 != 5;", "!=", "5", "5"}
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
        test(exp_stmt->expression->expression_type == INFIX_EXPRESSION,
            "Expected to find expression of type INFIX_EXPRESSION, found %s\n",
            get_expression_type_name(exp_stmt->expression->expression_type));
        printf("Found INFIX_EXPRESSION\n");
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
        {"3 + 4 * 5 == 3 * 1 + 4 * 5", "((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))"}
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
        {"-foobar", "-", "foobar"}
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
        // {"return true;", "true"},
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

int
main(int argc, char **argv)
{
    test_let_stmt();
    // test_parser_errors();
    test_return_statement();
    test_identifier_expression();
    test_integer_literal_expression();
    test_parse_prefix_expression();
    test_parse_infix_expression();
    test_operator_precedence_parsing();
    test_string();
    printf("All tests passed\n");

}
