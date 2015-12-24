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

bf_options parse_arguments(int argc, char *argv[]);

#endif /* BF_ARGUMENTS_H */
