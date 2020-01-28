#ifndef FRAME_H
#define FRAME_H

#include "opcode.h"
#include "object.h"

typedef struct frame_t {
    monkey_compiled_fn_t *fn;
    size_t ip;
    size_t bp;
} frame_t;

frame_t *frame_init(monkey_compiled_fn_t *, size_t);
void frame_free(frame_t *);
instructions_t *get_frame_instructions(frame_t *);

#endif