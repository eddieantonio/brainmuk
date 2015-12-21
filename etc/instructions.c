#include <bf_compile.h>

/*
 * THIS IS NOT LINKED TO THE MAIN EXECUTABLE!!!
 *
 * Instead, the dissasembly of the .text section of this object file should be
 * examined with objdump or otool.
 */

void bf_add(struct bf_runtime_context context) {
    volatile uint8_t *p = context.universe;
    (*p)++;
}


void bf_sub(struct bf_runtime_context context) {
    volatile uint8_t *p = context.universe;
    (*p)--;
}

void bf_add_addr(struct bf_runtime_context context) {
    volatile uint8_t *p = context.universe;
    p++;
}

void bf_sub_addr(struct bf_runtime_context context) {
    volatile uint8_t *p = context.universe;
    p--;
}

void bf_loop(struct bf_runtime_context context) {
    volatile uint8_t *p = context.universe;
    while (*p) {
        *p = '0';
    }
}

void bf_output(struct bf_runtime_context context) {
    volatile uint8_t *p = context.universe;

    context.output_byte(*p);
}

void bf_input(struct bf_runtime_context context) {
    volatile uint8_t *p = context.universe;

    *p = context.input_byte();
}
