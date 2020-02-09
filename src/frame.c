#include <err.h>
#include <stdlib.h>

#include "frame.h"

frame_t *
frame_init(monkey_closure_t *cl, size_t bp)
{
    frame_t *frame;
    frame = malloc(sizeof(*frame));
    if (frame == NULL)
        err(EXIT_FAILURE, "malloc failed");
    frame->cl = (monkey_closure_t *) copy_monkey_object((monkey_object_t *) cl);
    if (frame->cl == NULL)
        fprintf(stderr, "cl is null\n");
    frame->ip = 0;
    frame->bp = bp;
    return frame;
}

void
frame_free(frame_t *frame)
{
    free_monkey_object(frame->cl);
    free(frame);
}

instructions_t *
get_frame_instructions(frame_t *frame)
{
    return frame->cl->fn->instructions;
}