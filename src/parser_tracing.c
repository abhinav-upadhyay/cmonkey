#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser_tracing.h"

static size_t trace_level = 0;
static const char trace_indent_placeholder = '\t';

static char *
indent_level(void)
{
    if (trace_level == 0)
        return strdup("");
    size_t i;
    char *str = malloc(trace_level);
    if (str == NULL)
        return NULL;
    for (i = 0; i < trace_level; i++) {
        str[i] = trace_indent_placeholder;
    }
    str[trace_level - 1] = 0;
    return str;
}

static void
trace_print(char *string)
{
    char *indent = indent_level();
    if (indent == NULL)
        return;
    printf("%s%s\n", indent, string);
    free(indent);
}

static void
inc_indent(void)
{
    trace_level++;
}

static void
dec_indent(void)
{
    trace_level--;
}

const char *
trace(const char *msg)
{
    inc_indent();
    char *trace_msg = NULL;
    asprintf(&trace_msg, "BEGIN %s", msg);
    if (trace_msg == NULL)
        return NULL;
    trace_print(trace_msg);
    free(trace_msg);
    return msg;
}

void
untrace(const char *msg)
{
    char *untrace_msg = NULL;
    asprintf(&untrace_msg, "END %s", msg);
    if (untrace_msg == NULL) {
        dec_indent();
        return;
    }
    trace_print(untrace_msg);
    free(untrace_msg);
    dec_indent();
}