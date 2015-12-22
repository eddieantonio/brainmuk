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

TEST parses_filename() {
    /* Only filename; no other arguments. */
    bf_options options = parse_arguments(2, (char *[]) {
            "brainmuk", "hi.bf", NULL
    });
    ASSERT_FALSEm("got null for filename", options.filename == NULL);
    ASSERT_STR_EQm("Unexpected filename", "hi.bf", options.filename);

    /* Flags, and then other filename. */
    options = parse_arguments(3, (char *[]) {
            "brainmuk", "-m4g", "hello.bf", NULL
    });
    ASSERT_FALSEm("got null for filename", options.filename == NULL);
    ASSERT_STR_EQm("Unexpected filename", "hello.bf", options.filename);

    /* Filename, and then flags. */
    options = parse_arguments(3, (char *[]) {
            "brainmuk", "heya.bf", "-m4g", NULL
    });
    ASSERT_FALSEm("got null for filename", options.filename == NULL);
    ASSERT_STR_EQm("Unexpected filename", "heya.bf", options.filename);

    /* Two-part arugments, then filename. */
    options = parse_arguments(4, (char *[]) {
            "brainmuk", "-m", "4gb", "heya.bf",  NULL
    });
    ASSERT_FALSEm("got null for filename", options.filename == NULL);
    ASSERT_STR_EQm("Unexpected filename", "heya.bf", options.filename);

    /* Two filenames -- take the first */
    options = parse_arguments(3, (char *[]) {
            "brainmuk", "foo.bf", "bar.bf", NULL
    });
    ASSERT_FALSEm("got null for filename", options.filename == NULL);
    ASSERT_STR_EQm("Unexpected filename", "foo.bf", options.filename);

    PASS();
}

TEST parses_absence_of_filename() {
    bf_options options = parse_arguments(1, (char *[]) {
            "brainmuk", NULL
    });

    ASSERT(options.filename == NULL);

    options = parse_arguments(2, (char *[]) {
            "brainmuk", "-m4g",  NULL
    });

    ASSERT(options.filename == NULL);

    PASS();
}


SUITE(argument_parsing_suite) {
    RUN_TEST(parses_unsuffixed_minimum_size);
    RUN_TEST(parses_suffixed_minimum_size);
    RUN_TEST(parses_filename);
    RUN_TEST(parses_absence_of_filename);
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
    });

    ASSERT_EQ_FMTm("- did not decrement data", 0xFF, universe[0], "%hhu");

    PASS();
}

TEST compiles_address_increment() {
    bf_compile_result result = bf_compile(">+", memory);
    ASSERT_EQm("Failed to compile", result.status, BF_COMPILE_SUCCESS);
    ASSERT_EQm("Unexpected start address",
            (uint8_t *) result.program, memory);

    result.program((struct bf_runtime_context) {
        .universe = universe,
    });

    ASSERT_EQ_FMTm("> did not increment pointer", 0, universe[0], "%hhu");
    ASSERT_EQ_FMTm("+ did not increment data", 1, universe[1], "%hhu");

    PASS();
}

TEST compiles_address_decrement() {
    bf_compile_result result = bf_compile("<+", memory);
    ASSERT_EQm("Failed to compile", result.status, BF_COMPILE_SUCCESS);
    ASSERT_EQm("Unexpected start address",
            (uint8_t *) result.program, memory);

    result.program((struct bf_runtime_context) {
        /* NOTE! **Intentionally** offset the universe by one! */
        .universe = universe + 1,
    });

    ASSERT_EQ_FMTm("< did not decrement pointer", 0, universe[1], "%hhu");
    ASSERT_EQ_FMTm("+ did not increment data", 1, universe[0], "%hhu");

    PASS();
}

#define NOT_WRITTEN 0x100
static int output = NOT_WRITTEN;
static void dummy_output(char octet) {
    output = octet;
}

TEST compiles_output() {
    output = NOT_WRITTEN;

    /* Shuffle the data pointer around to ensure output abides. */
    bf_compile_result result = bf_compile(">+++<>.", memory);
    ASSERT_EQm("Failed to compile", result.status, BF_COMPILE_SUCCESS);
    ASSERT_EQm("Unexpected start address",
            (uint8_t *) result.program, memory);

    result.program((struct bf_runtime_context) {
        .universe = universe,
        .output_byte = dummy_output,
    });

    ASSERT_FALSEm("Output never called", output == NOT_WRITTEN);
    ASSERT_EQ_FMTm("Unexected value written", 3, output, "%d");

    PASS();
}

#define DETERMINISTIC_INPUT 42
static char dummy_input(void) {
    return DETERMINISTIC_INPUT;
}

TEST compiles_input() {
    /* Shuffle the data pointer around to ensure input abides; change its
     * data too to check if input actually worked... */
    bf_compile_result result = bf_compile(">+++<>,", memory);
    ASSERT_EQm("Failed to compile", result.status, BF_COMPILE_SUCCESS);
    ASSERT_EQm("Unexpected start address",
            (uint8_t *) result.program, memory);

    result.program((struct bf_runtime_context) {
        .universe = universe,
        .input_byte = dummy_input,
    });

    ASSERT_FALSEm("Input never called on correct address",
            universe[1] == 3);
    ASSERT_EQ_FMTm("Unexected value input",
            DETERMINISTIC_INPUT, universe[1], "%hhu");

    PASS();
}

TEST compiles_branch_skip() {
    output = NOT_WRITTEN;

    /* A branch on zero will skip what's inside. */
    bf_compile_result result = bf_compile("[,.]>+", memory);
    ASSERT_EQm("Failed to compile", result.status, BF_COMPILE_SUCCESS);
    ASSERT_EQm("Unexpected start address",
            (uint8_t *) result.program, memory);

    result.program((struct bf_runtime_context) {
        .universe = universe,
        .output_byte = dummy_output,
        .input_byte = dummy_input,
    });

    /* Checks if either input or output were called. */
    ASSERT_FALSEm("Failed to skip input", universe[0] == DETERMINISTIC_INPUT);
    ASSERTm("Failed to skip output", output == NOT_WRITTEN);
    ASSERT_EQ_FMTm("Failed to jump to end", 1, universe[1], "%hhu");

    PASS();
}

TEST compiles_branch_instructions() {
    output = NOT_WRITTEN;

    /* Moves 0xFF from the first cell to the second. */
    bf_compile_result result = bf_compile("-[->+<]", memory);
    ASSERT_EQm("Failed to compile", result.status, BF_COMPILE_SUCCESS);
    ASSERT_EQm("Unexpected start address",
            (uint8_t *) result.program, memory);

    result.program((struct bf_runtime_context) {
        .universe = universe,
        .output_byte = dummy_output,
        .input_byte = dummy_input,
    });

    /* Checks if either input or output were called. */
    ASSERT_EQ_FMTm("Unexpected value", 0x00, universe[0], "%hhu");
    ASSERT_EQ_FMTm("Unexpected value", 0xff, universe[1], "%hhu");

    PASS();
}

TEST compiles_nested_branches() {
    output = NOT_WRITTEN;

    /* Really silly; the inner "loop" outputs, and ends both loops. */
    bf_compile_result result = bf_compile("+[[.-]]", memory);
    ASSERT_EQm("Failed to compile", result.status, BF_COMPILE_SUCCESS);
    ASSERT_EQm("Unexpected start address",
            (uint8_t *) result.program, memory);

    result.program((struct bf_runtime_context) {
        .universe = universe,
        .output_byte = dummy_output,
    });

    /* Checks if either input or output were called. */
    ASSERT_FALSEm("Did not call output", output == NOT_WRITTEN);
    ASSERT_EQ_FMTm("Unexpected value", 0, universe[0], "%hhu");

    PASS();
}

SUITE(compile_suite) {
    GREATEST_SET_SETUP_CB(setup_compile, NULL);
    GREATEST_SET_TEARDOWN_CB(teardown_compile, NULL);

    RUN_TEST(compiles_simple_addition);
    RUN_TEST(compiles_simple_subtraction);
    RUN_TEST(compiles_address_increment);
    RUN_TEST(compiles_address_decrement);
    RUN_TEST(compiles_output);
    RUN_TEST(compiles_input);
    RUN_TEST(compiles_branch_skip);
    RUN_TEST(compiles_branch_instructions);
    RUN_TEST(compiles_nested_branches);
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
