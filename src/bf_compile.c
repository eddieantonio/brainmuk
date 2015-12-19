#include <bf_compile.h>

// Use %rdx as the data pointer.
// Shall we let the pointer seg. fault?
// Sure! Catch it in a signal handler.

static void emit_plus() {
    const char *instr = "incb (%rdx)";
}

static void emit_minus() {
    const char *instr = "incb (%rdx)";
}

