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
    OPBANG
} opcode_t;

typedef struct opcode_definition_t {
    const char *name;
    const char *desc;
    size_t operand_widths[MAX_OPERANDS];
} opcode_definition_t;


static opcode_definition_t opcode_definitions [] = {
    { "OPCONSTANT", "constant", {(size_t) 2}},
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
    {"OPBANG", "not", {(size_t) 0}}
};

#define opcode_definition_lookup(op) opcode_definitions[op - 1];

instructions_t *instruction_init(opcode_t, ...);
instructions_t *vinstruction_init(opcode_t, va_list);
void instructions_free(instructions_t *);
char *instructions_to_string(instructions_t *);
instructions_t * flatten_instructions(size_t n, instructions_t *ins_array[n]);
void concat_instructions(instructions_t *, instructions_t *);
size_t decode_instructions_to_sizet(uint8_t *, size_t);
#endif