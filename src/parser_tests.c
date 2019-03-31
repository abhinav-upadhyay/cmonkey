#include <assert.h>
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
    printf("Testing integer literal expression\n");
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
    printf("All tests passed for integerl literal\n");
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
test_let_stmt()
{
    const char *input = "let x = 5;\n"\
    "let y = 10;\n"\
    "let foobar = 838383;\n";
    print_test_separator_line();
    printf("Testing let statements\n");
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    check_parser_errors(parser);
    test(program != NULL, "parse_program returned NULL\n");
    printf("program parsed successfully\n");
    test(program->nstatements == 3, \
        "program does not have 3 statements, found %zu ements", \
        program->nstatements);
    printf("matched the number of statements\n");
    const char *tests[] = {
        "x",
        "y",
        "foobar"
    };
    for (int i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
        statement_t *stmt = program->statements[i];
        node_t stmt_node = stmt->node;
        char *stmt_token_literal = stmt_node.token_literal(stmt);
        test(strcmp(stmt_token_literal, "let") == 0, \
            "expected token literal to be \"let\", found %s", \
            stmt_token_literal);
        printf("matched let statement literal\n");
        test(stmt->statement_type == LET_STATEMENT, \
            "Expected statement type to be LET_STATEMENT, found %s", \
            get_statement_type_name(stmt->statement_type));
        printf("matched let statement type\n");
        letstatement_t *let_stmt = (letstatement_t *) stmt;
        identifier_t *name = let_stmt->name;
        char *name_literal = name->expression.node.token_literal(&name->expression.node);
        test(strcmp(name_literal, tests[i]) == 0, \
            "let_stmt.name.token_literal() not %s, got %s", tests[i], name_literal);
        printf("matched token literal\n");
        test(strcmp(name->value, tests[i]) == 0, "let_stmt.name.value not %s, got %s", tests[i], name->value);
        printf("matched identifier\n");
    }
    program_free(program);
    parser_free(parser);
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
test_parse_prefix_expression()
{
    typedef struct test_input {
        const char *input;
        const char *operator;
        long value;
    } test_input;

    test_input tests[] = {
        {"!5", "!", 5},
        {"-15", "-", 15}
    };

    size_t ntests = sizeof(tests)/sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        test_input test = tests[i];
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
        test_integer_literal_value(prefix_exp->right, test.value);
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
    print_test_separator_line();
    printf("Testing integer literal expression\n");
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

    integer_t *int_exp = (integer_t *) exp_stmt->expression;
    test(int_exp->value == 5,
        "expected the identifier value to be 5, found %ld", int_exp->value);
    printf("Matched the value of the identifier\n");

    char *_token_literal = int_exp->expression.node.token_literal(int_exp);
    test(strcmp(_token_literal, "5") == 0,
        "expected identifier token literal to be 5, found %s",
        _token_literal);
    printf("Matched the value of the token literal for the integer expression\n");
    printf("integer expression parsing test passed\n");
    program_free(program);
    parser_free(parser);

}

static void
test_return_statement()
{
    const char *input = "return 5;\n"\
    "return 10;\n"\
    "return 9988332;";
    print_test_separator_line();
    printf("Testing return statement\n");
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    check_parser_errors(parser);
    test(program != NULL, "parse_program failed\n");
    printf("program parsed successfully\n");
    test(program->nstatements == 3, "expected program to have 3 statements, found %zu\n",
        program->nstatements);
    printf("parsed correct number of statements\n");
    for (int i = 0; i < program->nstatements; i++) {
        statement_t *stmt = program->statements[i];
        node_t stmt_node = stmt->node;
        char *tok_literal = stmt_node.token_literal(stmt);
        test(strcmp(tok_literal, "return") == 0, "expected token literal to be \"return\"," \
            " found \"%s\"\n", tok_literal);
        printf("matched token literal for return statement\n");
        test(stmt->statement_type == RETURN_STATEMENT, "expected statement type to be %s," \
            "found %s\n", get_statement_type_name(RETURN_STATEMENT),
            get_statement_type_name(stmt->statement_type));
        printf("matched statement type for return\n");
    }
    program_free(program);
    parser_free(parser);
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
    parse_program(parser);
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
    // test_string();
    printf("All tests passed\n");

}
