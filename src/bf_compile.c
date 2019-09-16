#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <bf_compile.h>

/**
 * Loops may only be nested upto this length.
 */
#define MAX_NESTING_DEPTH   128
/**
 * We're not in any loop!
 */
#define NOT_IN_LOOP         -1

/**
 * (for bf_program_text) the allocation is of an unknown size.
 */
#define INDETERMINATE_SPACE 0

/* Context for outputing a loop. */
struct loop_context {
    /* Offset of the loop set up. */
    size_t loop_top_offset;
    /* Offset of the instruction to overwrite. */
    size_t placeholder_address_offset;
    /* Offset of the instruction to overwrite. */
    size_t loop_body_offset;
};

/* Conventions:
 *
 *  %rbx:
 *      contains uint8_t *p.
 *  0x10(%ebp):
 *      contains uint8_t *universe
 *  -0x10(%ebp):
 *      contains save space for %rbx
 */
static const uint8_t function_prologue[] = {
    /* Set up our stack frame... */
    0x55,                   // pushq    %rbp
    0x48, 0x89, 0xe5,       // movq     %rsp, %rbp
    /* Add 16 bytes of scratch space (need to be 16 byte-aligned). */
    0x48, 0x83, 0xec, 0x10, // subq     $0x10, %rsp

    /* Save %rbx. */
    0x48, 0x89, 0x5d, 0xf8, // movq     %rbx, -0x8(%rbp)

    /* $rbx = (uint8_t *) universe */
    0x48, 0x8d, 0x45, 0x10, // leaq     0x10(%rbp), %rax
    /* %rax = p */
    0x48, 0x8b, 0x18,       // movq     (%rax), %rbx
};

static const uint8_t function_epilogue[] = {
    /* Restore %rbx. */
    0x48, 0x8b, 0x5d, 0xf8, // movq	    0x8(%rbp), %rbx

    /* Relinquish our stack frame. */
    0x48, 0x83, 0xc4, 0x10, // addq     $0x10, %rsp
    0x5d,                   // popq     %rbp
    0xc3,                   // retq
};

static const uint8_t increment_memory[] = {
    /* (*p)++ */
    0xfe, 0x03              // incb (%rbx)
};

static const uint8_t decrement_memory[] = {
    /* (*p)-- */
    0xfe, 0x0b              // decb (%rbx)
};

static const uint8_t increment_data_pointer[] = {
    /* (*p)++ */
    0x48, 0xff, 0xc3        // incq %rbx
};

static const uint8_t decrement_data_pointer[] = {
    /* (*p)-- */
    0x48, 0xff, 0xcb        // decq %rbx
};

static const uint8_t output_byte[] = {
    /* prepare first argument (%edi = *p). */
    0x8a, 0x13,             // movb     (%rbx), %dl
    0x0f, 0xbe, 0xfa,       // movsbl   %dl, %edi

    /* do indirect call to output_byte(). */
    0x48, 0x8d, 0x45, 0x10, // leaq     0x10(%rbp), %rax
    0x48, 0x8b, 0x40, 0x08, // movq     0x8(%rax), %rax
    0xff, 0xd0,             // callq    *%rax
};

static const uint8_t input_byte[] = {
    /* do indirect call to input_byte(). */
    0x48, 0x8d, 0x4d, 0x10, // leaq     0x10(%rbp), %rcx
    0xff, 0x51, 0x10,       // callq    *0x10(%rax)

    /* Write the input back (returned by input_byte() in %al). */
    0x88, 0x03,             // movb     %al, (%rbx)
};

static const uint8_t loop_top[] = {
    /* Compare *p against 0. */
    0x8a, 0x0b,                         // movb (%rbx), %cl
    0x80, 0xf9, 0x00,                   // cmpb $0x0, %cl
    /* Skip if 0. */
    0x0f, 0x84, 0xff, 0xff, 0xff, 0xff, // je   [PLACEHOLDER]
};

static const uint8_t loop_bottom[] = {
    /* Unconditionally jump to loop start. */
    0xe9, 0xff, 0xff, 0xff, 0xff,       // jmp  [PLACEHOLDER]
};

/**
 * Macro that greatly simplifies cloning and concatenating machine code into
 * the address space.
 */
#define append_snippet(snippet)                       \
    do {                                             \
        memcpy(space + i, snippet, sizeof(snippet)); \
        i += sizeof(snippet);                        \
    } while (0)

static size_t start_loop(uint8_t *space, size_t i, struct loop_context *ctx) {
    /* The instruction starts here.... */
    ctx->loop_top_offset = i;
    append_snippet(loop_top);

    /* The address to overwrite is here... */
    ctx->placeholder_address_offset = i - sizeof(int32_t);
    ctx->loop_body_offset = i;

    return i;
}

static void patch_with(uint8_t* location, int32_t amount) {
    /* Ensure that the patch location is filled with the placeholder. */
    for (size_t i = 0; i < sizeof(int32_t); i++) {
        assert(location[i] == 0xFF);
    }

    /* Copy the address, byte-by-byte. */
    memcpy(location, &amount, sizeof(int32_t));
}

/* This is wrapped as a function to do type casting... */
static int32_t calc_offset(long from, long to) {
    return to - from;
}

static size_t end_loop(uint8_t *space, size_t i, struct loop_context *ctx) {
    assert(i > ctx->loop_top_offset);

    /* Write snippet such that i is pointing to the NEXT instruction. */
    append_snippet(loop_bottom);

    /* Patch the top of the loop. */
    patch_with(space + ctx->placeholder_address_offset,
            calc_offset(ctx->loop_body_offset, i));

    /* Patch the bottom of the loop. */
    uint8_t *loop_bottom_addr = space + (i - sizeof(int32_t));
    patch_with(loop_bottom_addr,
            calc_offset(i, ctx->loop_top_offset));

    return i;
}

static bf_compile_result error_status(enum bf_compile_status status) {
    return (bf_compile_result) {
        .status = status,
        .program = NULL
    };
}

/*
 * Note: the input to bf_compile() MUST be null-terminated!
 */
bf_compile_result bf_compile_realloc(const char *source, bf_program_text * restrict text) {
    size_t i = 0;  // position in memory, relative to page start.
    int current_loop = NOT_IN_LOOP;
    struct loop_context contexts[MAX_NESTING_DEPTH];
    const char *current_instruction = source;
    uint8_t *space = text->space;
    size_t half_capacity = text->allocated_space / 2;

    /* TODO: deal with max size. */

    append_snippet(function_prologue);

    while (*current_instruction != '\0') {

        /* Resize if we're getting too big. */
        if (text->should_resize && i >= half_capacity) {
            size_t new_capacity = 4 * text->allocated_space;
            uint8_t *new_space = allocate_executable_space(new_capacity);
            if (new_space == NULL) {
                abort();
            }
            memcpy(new_space, space, i);
            free_executable_space(space, text->allocated_space);
            space = text->space = new_space;
            text->allocated_space = new_capacity;
            half_capacity = new_capacity / 2;
        }

        switch (*current_instruction) {
            case '+':
                append_snippet(increment_memory);
                break;
            case '-':
                append_snippet(decrement_memory);
                break;
            case '<':
                append_snippet(decrement_data_pointer);
                break;
            case '>':
                append_snippet(increment_data_pointer);
                break;
            case '[':
                current_loop++;
                assert(current_loop >= 0);

                /* Give up if the nesting depth is too deep. */
                if (current_loop >= MAX_NESTING_DEPTH) {
                    return error_status(BF_COMPILE_NESTING_TOO_DEEP);
                }

                i = start_loop(space, i, &contexts[current_loop]);
                break;

            case ']':
                if (current_loop == NOT_IN_LOOP) {
                    return error_status(BF_COMPILE_UNMATCHED_BRACKET);
                }

                assert(current_loop >= 0 && current_loop < MAX_NESTING_DEPTH);
                i = end_loop(space, i, &contexts[current_loop]);
                current_loop--;
                break;

            case '.':
                append_snippet(output_byte);
                break;
            case ',':
                append_snippet(input_byte);
                break;
        }

        current_instruction++;
    }

    append_snippet(function_epilogue);

    /* We have at least one dangling open bracket. */
    if (current_loop != NOT_IN_LOOP) {
        return error_status(BF_COMPILE_UNMATCHED_BRACKET);
    }

    return (bf_compile_result) {
        .status = BF_COMPILE_SUCCESS,
        .program = (program_t) space
    };
}

/**
 * Like bf_compile_realloc(), but the the program text is preallocated, and
 * will never change its size.
 */
bf_compile_result bf_compile_no_alloc(const char *source, uint8_t *space) {
    bf_program_text text = (bf_program_text) {
        .space = space,
        .allocated_space = INDETERMINATE_SPACE,
        .should_resize = false,
    };
    return bf_compile_realloc(source, &text);
}
