#include <stddef.h>
#include <string.h>

#include <bf_compile.h>

#if 0
static const uint8_t full_function[] = {
    /* Set up function... */
    0x55,                   // pushq    %rbp
    0x48, 0x89, 0xe5,       // movq     %rsp, %rbp

    /* Load address of... universe address into %eax. */
    0x48, 0x8d, 0x45, 0x10, // leaq     0x10(%rbp), %rax
    /* %rax = p */
    0x48, 0x8b, 0x00,       // movq     (%rax), %rax
    0x48, 0x89, 0x45, 0xf8, // movq     %rax, -0x8(%rbp)
    0x48, 0x8b, 0x45, 0xf8, // movq     -0x8(%rbp), %rax

    /* %cl = *p */
    0x8a, 0x08,             // movb     (%rax), %cl
    /* %cl++ */
    0x80, 0xc1, 0x01,       // addb     $0x1, %cl
    /* *p = %cl */
    0x88, 0x08,             // movb     %cl, (%rax)

    /* Clean-up. */
    0x5d,                   // popq     %rbp
    0xc3,                   // retq
};
#endif

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


/**
 * Macro that greatly simplifies cloning and concatenating machine code into
 * the address space.
 */
#define append_snippet(snippet)                       \
    do {                                             \
        memcpy(space + i, snippet, sizeof(snippet)); \
        i += sizeof(snippet);                        \
    } while (0)


/*
 * Note: the input to compile MUST be null-terminated!
 */
bf_compile_result bf_compile(const char *source, uint8_t *space) {
    size_t i = 0;

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
        }

        current_instruction++;
    }

    append_snippet(function_epilogue);

    return (bf_compile_result) {
        .status = BF_COMPILE_SUCCESS,
        .program = (program_t) space
    };
}
