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

#include "object.h"
#include "test_utils.h"

static void
test_string_hash_key(void)
{
    print_test_separator_line();
    printf("Testing hash key generation for string objects\n");
    monkey_string_t *hello1 = create_monkey_string("hello world", 11);
    monkey_string_t *hello2 = create_monkey_string("hello world", 11);
    monkey_string_t *diff1 = create_monkey_string("My name is johnny", 17);
    monkey_string_t *diff2 = create_monkey_string("My name is johnny", 17);
    size_t hello1_hash = hello1->object.hash((monkey_object_t *) hello1);
    size_t hello2_hash = hello2->object.hash((monkey_object_t *) hello2);
    size_t diff1_hash = diff1->object.hash((monkey_object_t *) diff1);
    size_t diff2_hash = diff2->object.hash((monkey_object_t *) diff2);

    test(hello1_hash == hello2_hash,
        "Hash of hello1 %zu, different from that of hello2 %zu\n",
        hello1_hash, hello2_hash);
    
    test(diff1_hash == diff2_hash,
        "Hash of diff1 %zu, different from that of diff2 %zu\n",
        diff1_hash, diff2_hash);
    
    test(hello1_hash != diff1_hash,
        "Hash of hello1 %zu same as that of diff1 %zu\n",
        hello1_hash, diff1_hash);
    free_monkey_object(hello1);
    free_monkey_object(hello2);
    free_monkey_object(diff1);
    free_monkey_object(diff2);
}

int
main(int argc, char **argv)
{
    test_string_hash_key();
}