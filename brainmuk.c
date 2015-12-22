#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <bf_alloc.h>
#include <bf_runtime.h>
#include <bf_arguments.h>
#include <bf_compile.h>
#include <bf_slurp.h>

static char* program_name = NULL;
static void run_program(program_t program, bf_options *options);
static void run_file(bf_options *options);

int main(int argc, char *argv[]) {
    program_name = argv[0];
    bf_options options = parse_arguments(argc, argv);

    /* Check if a file has been provided. */
    if (options.filename == NULL) {
        fprintf(stderr, "%s: Must provide one filename\n", argv[0]);
        usage_error(program_name);
    }
    run_file(&options);

    return 0;
}

static void run_file(bf_options *options) {
    char *contents = slurp(options->filename);

    /* Try to open the file... */
    if (contents == NULL) {
        fprintf(stderr, "%s: Could not open '%s': ",
                program_name, options->filename);
        perror(NULL);
        exit(-1);
    }
    const size_t exec_mem_size = sysconf(_SC_PAGESIZE);

    /* Get space for the stuff. */
    uint8_t *exec_mem = allocate_executable_space(exec_mem_size);

    /* Compile and forget the source. */
    bf_compile_result compilation = bf_compile(contents, exec_mem);
    unslurp(contents);

    if (compilation.status == BF_COMPILE_SUCCESS) {
        run_program(compilation.program, options);
        free_executable_space(exec_mem, exec_mem_size);
    } else {
        free_executable_space(exec_mem, exec_mem_size);
        fprintf(stderr, "%s: %s: compilation failed!\n",
                program_name, options->filename);
        exit(compilation.status);
    }
}

static void run_program(program_t program, bf_options *options) {
    /* Allocate the ENTIRE UNIVERSE and run. */
    uint8_t *universe = calloc(options->minimum_universe_size, sizeof(uint8_t));

    if (universe == NULL) {
        fprintf(stderr, "%s: could not create universe (%lu bytes): ",
                program_name, options->minimum_universe_size);
        perror(NULL);
        exit(-1);
    }

    program((struct bf_runtime_context) {
            .universe = universe,
            .output_byte = bf_runtime_output_byte,
            .input_byte = bf_runtime_input_byte
    });

    free(universe);
}
