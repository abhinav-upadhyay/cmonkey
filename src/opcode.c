#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "cmonkey_utils.h"
#include "opcode.h"

instructions_t *
vinstruction_init(opcode_t op, va_list ap)
{
    instructions_t *ins;
    size_t operand;
    opcode_definition_t op_def;
    ins = malloc(sizeof(*ins));
    if (ins == NULL)
        err(EXIT_FAILURE, "malloc failed");
    switch (op) {
    case OPCONSTANT:
    case OPJMPFALSE:
    case OPJMP:
    case OPSETGLOBAL:
    case OPGETGLOBAL:
    case OPARRAY:
    case OPHASH:
        // these opcodes need only one operand 2 bytes wide
        operand = va_arg(ap, size_t);
        uint8_t *boperand = size_t_to_uint8_be(operand, 2);
        ins->bytes = create_uint8_array(3, op, boperand[0], boperand[1]);
        ins->length = 3;
        ins->size = 3;
        free(boperand);
        return ins;
    case OPSETLOCAL:
    case OPGETLOCAL:
    case OPCALL:
    case OPGETBUILTIN:
        operand = va_arg(ap, size_t);
        boperand = size_t_to_uint8_be(operand, 1);
        ins->bytes = create_uint8_array(2, op, boperand[0]);
        ins->length = 2;
        ins->size = 2;
        free(boperand);
        return ins;
    case OPADD:
    case OPSUB:
    case OPMUL:
    case OPDIV:
    case OPPOP:
    case OPTRUE:
    case OPFALSE:
    case OPGREATERTHAN:
    case OPEQUAL:
    case OPNOTEQUAL:
    case OPMINUS:
    case OPBANG:
    case OPNULL:
    case OPINDEX:
    case OPRETURNVALUE:
    case OPRETURN:
        ins->bytes = create_uint8_array(1, op);
        ins->length = 1;
        ins->size = 1;
        return ins;
    default:
        op_def = opcode_definition_lookup(op);
        errx(EXIT_FAILURE, "Unsupported opcode %s", op_def.name);
    }
    return ins;
}

instructions_t *
instruction_init(opcode_t op, ...)
{
    va_list ap;
    va_start(ap, op);
    instructions_t *ins = vinstruction_init(op, ap);
    va_end(ap);
    return ins;
}

instructions_t *
copy_instructions(instructions_t *ins)
{
    instructions_t *ret;
    ret = malloc(sizeof(*ret));
    if (ret == NULL)
        err(EXIT_FAILURE, "malloc failed");
    ret->bytes = malloc(ins->length);
    if (ret->bytes == NULL)
        err(EXIT_FAILURE, "malloc failed");
    for (size_t i = 0; i < ins->length; i++)
        ret->bytes[i] = ins->bytes[i];
    ret->length = ins->length;
    ret->size = ins->size;
    return ret;
}

instructions_t *
flatten_instructions(size_t n, instructions_t *ins_array[n])
{
    size_t bytes_len = 0;
    uint8_t *bytes;
    instructions_t *flat_ins;
    for (size_t i = 0; i < n; i++)
        bytes_len += ins_array[i]->length;

    bytes = malloc(sizeof(*bytes) * bytes_len);
    size_t bytes_offset = 0;
    for (size_t i = 0; i < n; i++) {
        instructions_t *ins = ins_array[i];
        for (size_t j = 0; j < ins->length; j++)
            bytes[bytes_offset++] = ins->bytes[j];
    }
    flat_ins = malloc(sizeof(*flat_ins));
    if (flat_ins == NULL)
        err(EXIT_FAILURE, "malloc failed");
    flat_ins->bytes = bytes;
    flat_ins->length = bytes_len;
    return flat_ins;
}


void
instructions_free(instructions_t *ins)
{
    if (ins == NULL)
        return;
    if (ins->bytes)
        free(ins->bytes);
    free(ins);
}

char *
instructions_to_string(instructions_t *instructions)
{
    char *string = NULL;
    opcode_definition_t op_def;
    for (size_t i = 0; i < instructions->length; i++) {
        opcode_t op = (opcode_t) instructions->bytes[i];
        size_t operand;
        op_def = opcode_definition_lookup(op);
        switch (op) {
        case OPCONSTANT:
        case OPJMPFALSE:
        case OPJMP:
        case OPSETGLOBAL:
        case OPGETGLOBAL:
        case OPARRAY:
        case OPHASH:
            operand = be_to_size_t(instructions->bytes + i + 1, 2);
            if (string == NULL) {
                int retval = asprintf(&string, "%04zu %s %zu", i, op_def.name, operand);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
            } else {
                char *temp = NULL;
                int retval = asprintf(&temp, "%s\n%04zu %s %zu", string, i, op_def.name, operand);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
                free(string);
                string = temp;
            }
            i += 2;
            break;
        case OPSETLOCAL:
        case OPGETLOCAL:
        case OPCALL:
        case OPGETBUILTIN:
            operand = be_to_size_t(instructions->bytes + i + 1, 1);
            if (string == NULL) {
                int retval = asprintf(&string, "%04zu %s %zu", i, op_def.name, operand);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
            } else {
                char *temp = NULL;
                int retval = asprintf(&temp, "%s\n%04zu %s %zu", string, i, op_def.name, operand);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
                free(string);
                string = temp;
            }
            i++;
            break;
        case OPADD:
        case OPSUB:
        case OPMUL:
        case OPDIV:
        case OPTRUE:
        case OPFALSE:
        case OPGREATERTHAN:
        case OPEQUAL:
        case OPNOTEQUAL:
        case OPMINUS:
        case OPBANG:
        case OPNULL:
        case OPINDEX:
        case OPRETURN:
        case OPRETURNVALUE:
            if (string == NULL) {
                int retval = asprintf(&string, "%04zu %s", i, op_def.name);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
            } else {
                char *temp = NULL;
                int retval = asprintf(&temp, "%s\n%04zu %s", string, i, op_def.name);
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
                free(string);
                string = temp;
            }
            break;
        case OPPOP:
            if (string == NULL) {
                int retval = asprintf(&string, "%04zu %s", i, "OPPOP");
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
            } else {
                char *temp = NULL;
                int retval = asprintf(&temp, "%s\n%04zu %s", string, i, "OPPOP");
                if (retval == -1)
                    err(EXIT_FAILURE, "malloc failed");
                free(string);
                string = temp;
            }
            break;
        default:
            return "";
        }
    }
    return string;
}

void
concat_instructions(instructions_t *dst, instructions_t *src)
{
    if (dst->size == 0 || dst->size - dst->length < src->length) {
        dst->size = dst->size * 2 + src->length;
        dst->bytes = reallocarray(dst->bytes, dst->size, sizeof(*dst->bytes));
        if (dst->bytes == NULL)
            err(EXIT_FAILURE, "malloc failed");
    }
    for (size_t i = 0; i < src->length; i++)
        dst->bytes[dst->length++] = src->bytes[i];
}

size_t
decode_instructions_to_sizet(uint8_t *bytes, size_t nbytes)
{
    return be_to_size_t(bytes, nbytes);
}
