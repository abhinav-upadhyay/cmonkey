#include <assert.h>
#include "parser.h"
#include <string.h>
#include "ast.h"
#include "lexer.h"
#include "test_utils.h"


static void
test_parser_errors(void)
{
    const char *input = "let x 5;\n"\
    "let = 10;\n"\
    "let 838383;\n";
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
    test(program != NULL, "parse_program returned NULL\n");
    test(parser->errors != NULL, "expected to find 3 parsing errors, found 0\n");
    test(parser->errors->length == 3, "expected to find 3 errors, found %zu errors\n", parser->errors->length);
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


int
main(int argc, char **argv)
{
    const char *input = "let x = 5;\n"\
    "let y = 10;\n"\
    "let foobar = 838383;\n";
    lexer_t *lexer = lexer_init(input);
    parser_t *parser = parser_init(lexer);
    program_t *program = parse_program(parser);
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
        char *name_literal = name->node.token_literal(&name->node);
        test(strcmp(name_literal, tests[i]) == 0, \
            "let_stmt.name.token_literal() not %s, got %s", tests[i], name_literal);
        printf("matched token literal\n");
        test(strcmp(name->value, tests[i]) == 0, "let_stmt.name.value not %s, got %s", tests[i], name->value);
        printf("matched identifier\n");
    }
    test_parser_errors();
    printf("All tests passed\n");
    program_free(program);
    parser_free(parser);

}