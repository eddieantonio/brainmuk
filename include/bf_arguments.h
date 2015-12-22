#ifndef BF_ARGUMENTS_H
#define BF_ARGUMENTS_H

#include <stdio.h>
#include <stddef.h>

typedef struct {
    /**
     * Mimimum size of the universe in bytes.
     */
    size_t minimum_universe_size;
    char *filename;
} bf_options;

/**
 * Prints brainmuk usage summary.
 */
void usage(const char* program_name, FILE *stream);

/**
 * Prints usage and exits with failure.
 */
__attribute__((noreturn))
void usage_error(const char *program_name);

bf_options parse_arguments(int argc, char *argv[]);

#endif /* BF_ARGUMENTS_H */
