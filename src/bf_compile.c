#include <stddef.h>
#include <string.h>
#include <assert.h>

#include <bf_compile.h>

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

    /* $rbx = (uint8_t *) universe */
    0x48, 0x8d, 0x45, 0x10, // leaq     0x10(%rbp), %rax
    /* %rax = p */
    0x48, 0x8b, 0x18,       // movq     (%rax), %rbx
};

static const uint8_t function_epilogue[] = {
    /* Relinquish our stack frame. */
    0x48, 0x83, 0xc4, 0x10, // addq     $0x10, %rsp
    0x5d,                   // popq     %rbp
    0xc3,                   // retq
};

static const uint8_t increment_memory[] = {
    /* (*p)++ */
    0xfe, 0x03              // incb (%rax)
};

static const uint8_t decrement_memory[] = {
    /* (*p)-- */
    0xfe, 0x0b              // decb (%rax)
};

static const uint8_t increment_data_pointer[] = {
    /* (*p)++ */
    0x48, 0xff, 0xc3        // incq %rax
};

static const uint8_t decrement_data_pointer[] = {
    /* (*p)-- */
    0x48, 0xff, 0xcb        // decq %rax
};

static const uint8_t output_byte[] = {
    /* save data pointer (%rbx) */
    0x48, 0x89, 0x5d, 0xf8, // movq     %rbx, -0x8(%rbp)

    /* prepare first argument (%edi = *p). */
    0x8a, 0x13,             // movb     (%rbx), %dl
    0x0f, 0xbe, 0xfa,       // movsbl   %dl, %edi

    /* do indirect call to output_byte(). */
    0x48, 0x8d, 0x45, 0x10, // leaq     0x10(%rbp), %rax
    0x48, 0x8b, 0x40, 0x08, // movq     0x8(%rax), %rax
    0xff, 0xd0,             // callq    *%rax

    /* restore instruction pointer (%rbx) */
    0x48, 0x8b, 0x5d, 0xf8, // movq     -0x8(%rbp), %rbx
};

static const uint8_t input_byte[] = {
    /* save data pointer (%rbx) */
    0x48, 0x89, 0x5d, 0xf8, // movq     %rbx, -0x8(%rbp)

    /* do indirect call to input_byte(). */
    0x48, 0x8d, 0x4d, 0x10, // leaq     0x10(%rbp), %rcx
    0xff, 0x51, 0x10,       // callq    *0x10(%rax)

    /* restore instruction pointer (%rbx) */
    0x48, 0x8b, 0x5d, 0xf8, // movq     -0x8(%rbp), %rbx

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
    for (int i = 0; i < sizeof(int32_t); i++) {
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

/*
 * Note: the input to compile MUST be null-terminated!
 */
bf_compile_result bf_compile(const char *source, uint8_t *space) {
    size_t i = 0;
    int current_loop = -1;
    struct loop_context contexts[1];
    const char *current_instruction = source;

    /* TODO: deal with max size. */
    /* TODO: have nesting stack of labels. */

    append_snippet(function_prologue);

    while (*current_instruction != '\0') {
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
                if (current_loop > 0) {
                    return (bf_compile_result) {
                        .status = -1,
                        .program = NULL
                    };
                }
                i = start_loop(space, i, &contexts[current_loop]);
                break;
            case ']':
                assert(current_loop >= 0);
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

    return (bf_compile_result) {
        .status = BF_COMPILE_SUCCESS,
        .program = (program_t) space
    };
}
