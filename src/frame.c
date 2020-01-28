#include <err.h>
#include <stdlib.h>

#include "frame.h"

frame_t *
frame_init(monkey_compiled_fn_t *fn, size_t bp)
{
    frame_t *frame;
    frame = malloc(sizeof(*frame));
    if (frame == NULL)
        err(EXIT_FAILURE, "malloc failed");
    frame->fn = (monkey_compiled_fn_t *) copy_monkey_object((monkey_object_t *) fn);
    frame->ip = 0;
    frame->bp = bp;
    return frame;
}

void
frame_free(frame_t *frame)
{
    free_monkey_object(frame->fn);
    free(frame);
}

instructions_t *
get_frame_instructions(frame_t *frame)
{
    return frame->fn->instructions;
}