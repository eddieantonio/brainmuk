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

static const uint8_t function_prologue[] = {
    /* Set up our stack frame... */
    0x55,                   // pushq    %rbp
    0x48, 0x89, 0xe5,       // movq     %rsp, %rbp

    /* Load address of... universe address into %eax. */
    0x48, 0x8d, 0x45, 0x10, // leaq     0x10(%rbp), %rax
    /* %rax = p */
    0x48, 0x8b, 0x00,       // movq     (%rax), %rax
};

static const uint8_t function_epilogue[] = {
    /* Relinquish our stack frame. */
    0x5d,                   // popq     %rbp
    0xc3,                   // retq
};

static const uint8_t add_memory[] = {
    /* %cl = *p */
    0x8a, 0x08,             // movb     (%rax), %cl
    /* %cl++ */
    0x80, 0xc1, 0x01,       // addb     $0x1, %cl
    /* *p = %cl */
    0x88, 0x08,             // movb     %cl, (%rax)
};


/**
 * Macro that greatly simplifies cloning and concatenating machine code into
 * the address space.
 */
#define clone_snippet(snippet)                       \
    do {                                             \
        memcpy(space + i, snippet, sizeof(snippet)); \
        i += sizeof(snippet);                        \
    } while (0)


bf_compile_result bf_compile(const char *source, uint8_t *space) {
    size_t i = 0;

    clone_snippet(function_prologue);
    clone_snippet(add_memory);
    clone_snippet(function_epilogue);

    return (bf_compile_result) {
        .status = BF_COMPILE_SUCCESS,
        .program = (program_t) space
    };
}
