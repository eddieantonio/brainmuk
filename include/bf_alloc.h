#ifndef BF_ALLOC_H
#define BF_ALLOC_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * Allocates space whose memory protection allows for execution. Use this
 * space to write machine code into the current address space.
 *
 * @param   size    number of bytes to allocate
 * @return          a pointer to *at least* `size` bytes of executable space, or
 *                  NULL if allocation failed; errno might be informative.
 * @see     free_executable_space()
 */
uint8_t *allocate_executable_space(size_t size);

/**
 * Frees space allocated with allocate_executable_space
 *
 * @return  bool    true if succeeded; false otherwise.
 */
bool free_executable_space(uint8_t *space, size_t size);

#endif /* BF_ALLOC_H */
