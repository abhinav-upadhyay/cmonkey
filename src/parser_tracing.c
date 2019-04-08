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