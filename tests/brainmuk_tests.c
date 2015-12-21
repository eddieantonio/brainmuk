#include <string.h>
#include <unistd.h>

#include "greatest.h"

#include <brainmuk.h>
#include <bf_alloc.h>
#include <bf_compile.h>
#include <bf_slurp.h>

/*********************** tests for parse_arguments() ***********************/

#define KIBIBYTES(x)    ((size_t) (x) * (size_t) 1024)
#define MIBIBYTES(x)    ((size_t) (x) * KIBIBYTES(1024))
#define GIBIBYTES(x)    ((size_t) (x) * MIBIBYTES(1024))

TEST parses_unsuffixed_minimum_size() {
    /* Assumes megabytes by default. */
    bf_options options = parse_arguments(2, (char *[]) {
            "brainmuk", "-m128", NULL
    });

    ASSERT_EQ_FMTm("-m128",
            MIBIBYTES(128), options.minimum_universe_size, "%lu");

    options = parse_arguments(2, (char *[]) {
            "brainmuk", "--minimum-universe=128", NULL
    });

    ASSERT_EQ_FMTm("--minimum-universe=128",
            MIBIBYTES(128), options.minimum_universe_size, "%lu");

    PASS();
}

TEST parses_suffixed_minimum_size() {
    bf_options options = parse_arguments(2, (char *[]) {
            "brainmuk", "-m4g", NULL
    });

    ASSERT_EQ_FMTm("-m4g",
            GIBIBYTES(4), options.minimum_universe_size, "%lu");

    options = parse_arguments(2, (char *[]) {
            "brainmuk", "--minimum-universe=64kb", NULL
    });

    ASSERT_EQ_FMTm("--minimum-universe=64k",
            KIBIBYTES(64), options.minimum_universe_size, "%lu");

    PASS();
}

/********************* tests for slurp() and unslurp() *********************/

TEST normal_file_can_be_slurped_and_unslurped() {
#define test_filename __FILE__ ".fixtures/normal"
    char *str = slurp(test_filename);

    ASSERTm("Could not slurp file: " test_filename , str != NULL);

    ASSERT_STR_EQm("Unexpected file contents for " test_filename,
           "Hi, I'm a normal file.\n", str);

    ASSERTm("Could not unslurp " test_filename, unslurp(str));

    PASS();
#undef test_filename
}

/******************* tests for allocate_executable_space *******************/

static const uint8_t X86_RET = 0xC3;
static const uint8_t X86_NOP = 0x90;

TEST space_returned_can_be_executed_and_freed(void) {
    uint8_t* space = allocate_executable_space(1);

    ASSERT_OR_LONGJMPm("Allocation failed", space != NULL);

    /* Writing a single return instruction creates an executable function. */
    space[0] = X86_RET;

    /* Call the function. */
    ((void (*)(void)) space)();

    /* At this point, a seg fault would indicate failure. */

    /* Otherwise, deallocate! */
    ASSERTm("Could not free space", free_executable_space(space));

    PASS();
}

TEST space_returned_is_given_size(void) {
    /* Allocate some biggish size of executable memory (bigger than the size
     * of one page, at least). */
    size_t base_size = getpagesize() * 10 + 1;
    size_t size = base_size + 1;
    uint8_t* space = allocate_executable_space(size);

    ASSERT_OR_LONGJMPm("Allocation failed", space != NULL);

    /* Write a nop-sled! */
    memset(space, X86_NOP, base_size);
    /* at the end of the nop-sled, we'll return. */
    space[size - 1] = X86_RET;

    /* Call the function. */
    ((void (*)(void)) space)();

    /* At this point, a segmentation fault would indicate failure. */

    /* Otherwise, deallocate! */
    ASSERTm("Could not free space", free_executable_space(space));

    PASS();
}

/*************************** tests for compile() ***************************/

TEST compiles_simple_addition() {
    uint8_t universe[256];
    memset(universe, 0, sizeof(universe));

    uint8_t *memory = allocate_executable_space(getpagesize() - 1);
    ASSERT_FALSEm("Failed to allocate executable space", memory == NULL);

    bf_compile_result result = bf_compile("+", memory);
    ASSERT_EQm("Failed to compile", result.status, BF_COMPILE_SUCCESS);
    ASSERT_EQm("Unexpected program location",
            (uint8_t *) result.program, memory);

    result.program((struct bf_runtime_context) {
        .universe = universe,
        .output_byte = NULL,
        .input_byte = NULL
    });

    ASSERT_EQ_FMTm("+ did not increment data", 1, universe[0], "%hhu");

    ASSERTm("Could not free space", free_executable_space(memory));

    PASS();
}

TEST compiles_simple_subtraction() {
    uint8_t universe[256];
    memset(universe, 0, sizeof(universe));

    uint8_t *memory = allocate_executable_space(getpagesize() - 1);
    ASSERT_FALSEm("Failed to allocate executable space", memory == NULL);

    bf_compile_result result = bf_compile("-", memory);
    ASSERT_EQm("Failed to compile", result.status, BF_COMPILE_SUCCESS);
    ASSERT_EQm("Unexpected program location",
            (uint8_t *) result.program, memory);

    result.program((struct bf_runtime_context) {
        .universe = universe,
        .output_byte = NULL,
        .input_byte = NULL
    });

    ASSERT_EQ_FMTm("- did not decrement data", 0xFF, universe[0], "%hhu");

    ASSERTm("Could not free space", free_executable_space(memory));

    PASS();
}



/********************************** main() **********************************/

/* Add all the definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line arguments, initialization. */

    RUN_TEST(parses_unsuffixed_minimum_size);
    RUN_TEST(parses_suffixed_minimum_size);

    RUN_TEST(space_returned_can_be_executed_and_freed);
    RUN_TEST(space_returned_is_given_size);

    RUN_TEST(normal_file_can_be_slurped_and_unslurped);

    RUN_TEST(compiles_simple_addition);
    RUN_TEST(compiles_simple_subtraction);

    GREATEST_MAIN_END();        /* display results */
}
