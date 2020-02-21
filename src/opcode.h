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

#ifndef CODE_H
#define CODE_H

#include <stdarg.h>
#include <stdint.h>
#include "cmonkey_utils.h"

#define MAX_OPERANDS 16 //not strictly enforced but for convenience in opcode_definition_t

typedef struct instructions_t {
    uint8_t *bytes;
    size_t length;
    size_t size;
} instructions_t;

typedef enum opcode_t {
    OPCONSTANT = 1,
    OPADD,
    OPSUB,
    OPMUL,
    OPDIV,
    OPPOP,
    OPTRUE,
    OPFALSE,
    OPEQUAL,
    OPNOTEQUAL,
    OPGREATERTHAN,
    OPMINUS,
    OPBANG,
    OPJMPFALSE,
    OPJMP,
    OPNULL,
    OPSETGLOBAL,
    OPGETGLOBAL,
    OPARRAY,
    OPHASH,
    OPINDEX,
    OPCALL,
    OPRETURNVALUE,
    OPRETURN,
    OPSETLOCAL,
    OPGETLOCAL,
    OPGETBUILTIN,
    OPCLOSURE,
    OPGETFREE,
    OPCURRENTCLOSURE
} opcode_t;

typedef struct opcode_definition_t {
    const char *name;
    const char *desc;
    size_t operand_widths[MAX_OPERANDS];
} opcode_definition_t;


static opcode_definition_t opcode_definitions [] = {
    {"OPCONSTANT", "constant", {(size_t) 2}},
    {"OPADD", "+", {(size_t) 0}},
    {"OPSUB", "-", {(size_t) 0}},
    {"OPMUL", "*", {(size_t) 0}},
    {"OPDIV", "/", {(size_t) 0}},
    {"OPPOP", "pop", {(size_t) 0}},
    {"OPTRUE", "true", {(size_t) 0}},
    {"OPFALSE", "false", {(size_t) 0}},
    {"OPEQUAL", "==", {(size_t) 0}},
    {"OPNOTEQUAL", "!=", {(size_t) 0}},
    {"OPGREATERTHAN", ">", {(size_t) 0}},
    {"OPMINUS", "-", {(size_t) 0}},
    {"OPBANG", "not", {(size_t) 0}},
    {"OPJMPFALSE", "jump_if_false", {(size_t) 2}},
    {"OPJMP", "jump", {(size_t) 2}},
    {"OPNULL", "null", {(size_t) 0}},
    {"OPSETGLOBAL", "set_global", {(size_t) 2}},
    {"OPGETGLOBAL", "get_global", {(size_t) 2}},
    {"OPARRAY", "array", {(size_t) 2}},
    {"OPHASH", "hash", {(size_t) 2}},
    {"OPINDEX", "index", {(size_t) 0}},
    {"OPCALL", "call", {(size_t) 1}},
    {"OPRETURNVALUE", "reeturn_value", {(size_t) 0}},
    {"OPRETURN", "return", {(size_t) 0}},
    {"OPSETLOCAL", "set_local", {(size_t) 1}},
    {"OPGETLOCAL", "get_local", {(size_t) 1}},
    {"OPGETBUILTIN", "get_builtin", {(size_t) 1}},
    {"OPCLOSURE", "closure", {(size_t) 2, (size_t) 1}},
    {"OPGETFREE", "get_free", {(size_t) 1}},
    {"OPCURRENTCLOSURE", "current_closure", {(size_t) 0}}
};

#define opcode_definition_lookup(op) opcode_definitions[op - 1];
#define decode_instructions_to_sizet(bytes, nbytes) nbytes == 1? (bytes)[0]: be_to_size_t(bytes, nbytes)

instructions_t *instruction_init(opcode_t, ...);
instructions_t *vinstruction_init(opcode_t, va_list);
void instructions_free(instructions_t *);
char *instructions_to_string(instructions_t *);
instructions_t * flatten_instructions(size_t n, instructions_t *ins_array[n]);
void concat_instructions(instructions_t *, instructions_t *);
instructions_t *copy_instructions(instructions_t *);
#endif