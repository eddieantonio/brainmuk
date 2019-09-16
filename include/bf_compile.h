/**
 * This file is part of Brainmuk.
 * 2015 (c) eddieantonio. See LICENSE for details.
 */

#ifndef BF_COMPILE_H
#define BF_COMPILE_H

#include <stdint.h>

#include <bf_alloc.h>

enum bf_compile_status {
    /** Compilation was successful. */
    BF_COMPILE_SUCCESS = 0,
    /** Program has unmatched brackets. */
    BF_COMPILE_UNMATCHED_BRACKET,
    BF_COMPILE_NESTING_TOO_DEEP,
    /* A totally generic error. */
    BF_COMPILE_ERROR,
};

/**
 * Stores all runtime context.
 *
 * The context is:
 *
 *  - the "universe": all of the memory that could possibly be available.
 *  - output_byte: it should prints exactly one octet of output;
 *  - input_byte: it should return exactly one octet of input
 */
struct bf_runtime_context {
    uint8_t *universe;
    void (*output_byte)(uint8_t);
    uint8_t (*input_byte)();
};

/**
 * A brainmuk program pointer!
 *
 * You may call this with a valid bf_runtime_context.
 */
typedef void (*program_t)(struct bf_runtime_context);

/**
 * Tagged union that represents the result of compilation.
 */
typedef struct {
    enum bf_compile_status status;

    union {
        /** The compiled program as result of compilation. */
        program_t program;

        /** The location of an error. */
        struct {
            unsigned long err_line;
            unsigned long err_col;
        };
    };
} bf_compile_result;

/**
 * Compiles the null-terminated source text to the given space.
 * When this function returns BF_COMPILE_SUCCES
 *
 * @param char[]    null-terminated program source text
 * @param program_t freshly-created program space
 *
 * @return the compilation status.
 */
bf_compile_result bf_compile(const char *source, uint8_t *space);

/**
 * Like bf_compile(), but allows for the reallocation of the program text.
 *
 * @param char[]          null-terminated program source text
 * @param bf_program_text program text that may be resized during compilation
 *
 * @return the compilation status.
 */
bf_compile_result bf_compile_realloc(const char *source, bf_program_text *text);

#endif /* BF_COMPILE_H */
