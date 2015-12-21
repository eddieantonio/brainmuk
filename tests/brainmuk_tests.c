#include <assert.h>
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

SUITE(argument_parsing_suite) {
    RUN_TEST(parses_unsuffixed_minimum_size);
    RUN_TEST(parses_suffixed_minimum_size);
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

SUITE(slurp_suite) {
    RUN_TEST(normal_file_can_be_slurped_and_unslurped);
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

SUITE(allocate_executable_suite) {
    RUN_TEST(space_returned_can_be_executed_and_freed);
    RUN_TEST(space_returned_is_given_size);
}

/*************************** tests for compile() ***************************/

static uint8_t universe[256] = { 0 };
static uint8_t *memory = NULL;

/* Zeros the universe and allocates fresh executable memory. */
static void setup_compile(void *data) {
    memset(universe, 0, sizeof(universe));
    assert(memory == NULL && "(uint_8 *) memory in unexpected state");

    memory = allocate_executable_space(getpagesize() - 1);
    assert(memory != NULL && "Failed to allocate executable space");
}

/* Deallocates the executable space. */
static void teardown_compile(void *data) {
    assert(free_executable_space(memory) && "Could not free space");
    memory = NULL;
}

TEST compiles_simple_addition() {
    bf_compile_result result = bf_compile("+", memory);
    ASSERT_EQm("Failed to compile", result.status, BF_COMPILE_SUCCESS);
    ASSERT_EQm("Unexpected start address",
            (uint8_t *) result.program, memory);

    result.program((struct bf_runtime_context) {
        .universe = universe,
        .output_byte = NULL,
        .input_byte = NULL
    });

    ASSERT_EQ_FMTm("+ did not increment data", 1, universe[0], "%hhu");

    PASS();
}

TEST compiles_simple_subtraction() {
    bf_compile_result result = bf_compile("-", memory);
    ASSERT_EQm("Failed to compile", result.status, BF_COMPILE_SUCCESS);
    ASSERT_EQm("Unexpected start address",
            (uint8_t *) result.program, memory);

    result.program((struct bf_runtime_context) {
        .universe = universe,
        .output_byte = NULL,
        .input_byte = NULL
    });

    ASSERT_EQ_FMTm("- did not decrement data", 0xFF, universe[0], "%hhu");

    PASS();
}

SUITE(compile_suite) {
    GREATEST_SET_SETUP_CB(setup_compile, NULL);
    GREATEST_SET_TEARDOWN_CB(teardown_compile, NULL);

    RUN_TEST(compiles_simple_addition);
    RUN_TEST(compiles_simple_subtraction);
}


/********************************** main() **********************************/

/* Add all the definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line arguments, initialization. */

    RUN_SUITE(argument_parsing_suite);
    RUN_SUITE(slurp_suite);
    RUN_SUITE(allocate_executable_suite);
    RUN_SUITE(compile_suite);

    GREATEST_MAIN_END();        /* display results */
}
