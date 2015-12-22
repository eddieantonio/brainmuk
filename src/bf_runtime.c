#include <stdio.h>

#include <bf_runtime.h>

/**
 * A void wrapper for putchar.
 */
void bf_runtime_output_byte(uint8_t byte) {
    putchar(byte);
}

/**
 * @return  the character; if the stream has reached EOF, returns 0xFF.
 *          0xFF is a good value to use because it's relatively easy to
 *          compare to (simply add one), and it's an invalid UTF-8 byte.
 */
uint8_t bf_runtime_input_byte() {
    int c = getchar();
    if (c == EOF) {
        return 0xFF;
    }
    return c;
}
