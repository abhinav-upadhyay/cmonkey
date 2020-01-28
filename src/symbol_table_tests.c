#include <string.h>

#include "cmonkey_utils.h"
#include "symbol_table.h"
#include "test_utils.h"

static void
compare_symbols(symbol_t *expected, symbol_t *actual)
{
    test(strcmp(expected->name, actual->name) == 0, "Expected symbol name: %s, got %s\n",
        expected->name, actual->name);
    test(expected->index == actual->index, "Expected symbol index %u, got %u\n",
        expected->index, actual->index);
    test(expected->scope == actual->scope, "Expected scope %s, got %s\n",
        get_scope_name(expected->scope), get_scope_name(actual->scope));
}

static void
test_define(void)
{
    print_test_separator_line();
    printf("Testing symbol define\n");
    cm_hash_table *expected = cm_hash_table_init(string_hash_function, string_equals, NULL, free_symbol);
    cm_hash_table_put(expected, "a", symbol_init("a", GLOBAL, 0));
    cm_hash_table_put(expected, "b", symbol_init("b", GLOBAL, 1));
    cm_hash_table_put(expected, "c", symbol_init("c", LOCAL, 0));
    cm_hash_table_put(expected, "d", symbol_init("d", LOCAL, 1));
    cm_hash_table_put(expected, "e", symbol_init("e", LOCAL, 0));
    cm_hash_table_put(expected, "f", symbol_init("f", LOCAL, 1));

    symbol_table_t *global = symbol_table_init();
    symbol_t *s = symbol_define(global, "a");
    compare_symbols((symbol_t *) cm_hash_table_get(expected, "a"), s);
    
    s = symbol_define(global, "b");
    compare_symbols((symbol_t *) cm_hash_table_get(expected, "b"), s);

    symbol_table_t *first_local = enclosed_symbol_table_init(global);
    s = symbol_define(first_local, "c");
    compare_symbols((symbol_t *) cm_hash_table_get(expected, "c"), s);
    s = symbol_define(first_local, "d");
    compare_symbols((symbol_t *) cm_hash_table_get(expected, "d"), s);

    symbol_table_t *second_local = enclosed_symbol_table_init(first_local);
    s = symbol_define(second_local, "e");
    compare_symbols((symbol_t *) cm_hash_table_get(expected, "e"), s);
    s = symbol_define(second_local, "f");
    compare_symbols((symbol_t *) cm_hash_table_get(expected, "f"), s);


    cm_hash_table_free(expected);
    free_symbol_table(global);
    free_symbol_table(first_local);
    free_symbol_table(second_local);
}

static void
test_resolve_global(void)
{
    print_test_separator_line();
    printf("Testing global symbols resolution\n");
    symbol_table_t *global = symbol_table_init();
    symbol_define(global, "a");
    symbol_define(global, "b");

    symbol_t *expected[] = {
        symbol_init("a", GLOBAL, 0),
        symbol_init("b", GLOBAL, 1)
    };

    for (size_t i = 0; i < 2; i++) {
        symbol_t *sym = expected[i];
        symbol_t *result = symbol_resolve(global, sym->name);
        test(result != NULL, "No symbol found with name %s in the table\n", sym->name);
        compare_symbols(sym, result);
        free_symbol(sym);
    }
    free_symbol_table(global);
}

static void
test_resolve_local(void)
{
    print_test_separator_line();
    printf("Testing local symbol tables\n");
    symbol_table_t *global = symbol_table_init();
    symbol_define(global, "a");
    symbol_define(global, "b");
    symbol_table_t *first_local = enclosed_symbol_table_init(global);
    symbol_define(first_local, "c");
    symbol_define(first_local, "d");

    symbol_t *expected[] = {
        symbol_init("a", GLOBAL, 0),
        symbol_init("b", GLOBAL, 1),
        symbol_init("c", LOCAL, 0),
        symbol_init("d", LOCAL, 1)
    };

    for (size_t i = 0; i < 4; i++) {
        symbol_t *sym = expected[i];
        symbol_t *result = symbol_resolve(first_local, sym->name);
        test(result != NULL, "No symbol found with name %s in the table\n", sym->name);
        compare_symbols(sym, result);
        free_symbol(sym);
    }
    free_symbol_table(global);
    free_symbol_table(first_local);
}

static void
test_define_resolve_builtins(void)
{
    print_test_separator_line();
    printf("Testing symbol resolution for builtins\n");
    symbol_table_t *global = symbol_table_init();
    symbol_table_t *first_local = enclosed_symbol_table_init(global);
    symbol_table_t *second_local = enclosed_symbol_table_init(first_local);
    symbol_t *expected[] = {
        symbol_init("a", BUILTIN, 0),
        symbol_init("c", BUILTIN, 1),
        symbol_init("e", BUILTIN, 2),
        symbol_init("f", BUILTIN, 3)
    };
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        symbol_t *s = expected[i];
        symbol_define_builtin(global, i, s->name);
    }

    symbol_table_t *tables[] = {global, first_local, second_local};

    for (size_t i = 0; i < sizeof(tables) / sizeof(tables[0]); i++) {
        for (size_t j = 0; j < sizeof(expected) / sizeof(expected[0]); j++) {
            symbol_table_t *table = tables[i];
            symbol_t *sym = expected[j];
            symbol_t *resolved = symbol_resolve(table, sym->name);
            test(resolved != NULL, "Failed to resolve symbol %s\n", sym->name);
            compare_symbols(sym, resolved);
        }
    }
    free_symbol_table(global);
    free_symbol_table(first_local);
    free_symbol_table(second_local);
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++)
        free_symbol(expected[i]);
}

static void
test_resolve_nested_local(void)
{
    print_test_separator_line();
    printf("Testing nested local symbol tables\n");
    symbol_table_t *global = symbol_table_init();
    symbol_define(global, "a");
    symbol_define(global, "b");

    symbol_table_t *first_local = enclosed_symbol_table_init(global);
    symbol_define(first_local, "c");
    symbol_define(first_local, "d");

    symbol_table_t *second_local = enclosed_symbol_table_init(first_local);
    symbol_define(second_local, "e");
    symbol_define(second_local, "f");

    typedef struct testcase {
        symbol_table_t *table;
        symbol_t *expected_symbols[4];
    } testcase;

    testcase tests[] = {
        {
            first_local,
            {
                symbol_init("a", GLOBAL, 0),
                symbol_init("b", GLOBAL, 1),
                symbol_init("c", LOCAL, 0),
                symbol_init("d", LOCAL, 1)
            }
        },
        {
            second_local,
            {
                symbol_init("a", GLOBAL, 0),
                symbol_init("b", GLOBAL, 1),
                symbol_init("e", LOCAL, 0),
                symbol_init("f", LOCAL, 1)
            }
        }
    };
    size_t ntests = sizeof(tests)/sizeof(tests[0]);
    for (size_t i = 0; i < ntests; i++) {
        testcase t = tests[i];
        for (size_t j = 0; j < 4; j++) {
            symbol_t *resolved = symbol_resolve(t.table, t.expected_symbols[j]->name);
            test(resolved != NULL, "Failed to resolv symbol %s\n", t.expected_symbols[j]->name);
            compare_symbols(t.expected_symbols[j], resolved);
            free_symbol(t.expected_symbols[j]);
        }
    }
    free_symbol_table(global);
    free_symbol_table(first_local);
    free_symbol_table(second_local);
}

int
main(int argc, char **argv)
{
    test_define();
    test_resolve_global();
    test_resolve_local();
    test_resolve_nested_local();
    test_define_resolve_builtins();
    return 0;
}