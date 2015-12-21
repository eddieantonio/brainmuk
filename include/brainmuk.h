#ifndef BRAINMUK_H
#define BRAINMUK_H

#include <stddef.h>

typedef struct {
    /**
     * Mimimum size of the universe in bytes.
     */
    size_t minimum_universe_size;
} bf_options;

/**
 * Main entry point for the program.
 */
int brainmuk(int argc, char *argv[]);

bf_options parse_arguments(int argc, char *argv[]);

#endif /* BRAINMUK_H */
