#include <assert.h>
#include "parser.h"
#include <string.h>
#include "ast.h"
#include "lexer.h"
#include "test_utils.h"

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
    printf("All tests passed\n");
}