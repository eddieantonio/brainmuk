#ifndef BF_RUNTIME_H
#define BF_RUNTIME_H

#include <stdint.h>

/**
 * Functions bundled for use in the runtime.
 */

/**
 * A void wrapper for putchar.
 */
void bf_runtime_output_byte(uint8_t byte);

/**
 * @return  the character; if the stream has reached EOF, returns 0xFF.
 *          0xFF is a good value to use because it's relatively easy to
 *          compare to (simply sub one from zero), and it's an invalid UTF-8
 *          byte.
 */
uint8_t bf_runtime_input_byte();

#endif /* BF_RUNTIME_H */
