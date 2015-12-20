#ifndef BF_SLURP_H
#define BF_SLURP_H

#include <stdbool.h>

/**
 * @return  The null-terminated contents of the named file. If this fails for
 *          any reason, returns NULL.
 */
char *slurp(const char* filename);

/**
 * Deallocates memory allocated by unslurp().
 * @return true when successful.
 */
bool unslurp(char *memory);

#endif /* BF_SLURP_H */
