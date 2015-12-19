/**
 * This file is part of Brainmuk.
 * 2015 (c) eddieantonio. See LICENSE for details.
 */

#ifndef BF_COMPILE_H
#define BF_COMPILE_H

#include <stdint.h>

typedef enum {
    /** Compilation was successful. */
    BF_COMPILE_SUCCESS = 0,
    /** The given program space looks invalid. */
    BF_COMPILE_SPACE_INVALID,
    /** Ran out of memory. */
    BF_COMPILE_INSUFFICIENT_MEMORY,
    /** Program exhibited invalid nesting. */
    BF_COMPILE_NESTING_ERROR,
} bf_compile_status;

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
    void (*output_byte)(char);
    char (*input_byte)();
};

/**
 * A brainmuk program pointer!
 *
 * You may call this with a valid bf_runtime_context.
 */
typedef void (*program_t)(struct bf_runtime_context);

/**
 * Compiles the null-terminated source text to the given space.
 * When this function returns BF_COMPILE_SUCCES
 *
 * @param char[]    null-terminated programm source text
 * @param program_t freshly-created program space
 *
 * @return the compilation status.
 */
extern bf_compile_status bf_compile(const char *source, program_t space);

#endif /* BF_COMPILE_H */
