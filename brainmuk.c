#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <bf_alloc.h>
#include <bf_version.h>
#include <bf_runtime.h>
#include <bf_arguments.h>
#include <bf_compile.h>
#include <bf_slurp.h>

#define REPL_LINE_LENGTH 1024

static char* program_name = NULL;
static void run_file(bf_options *options);
static void repl(bf_options *options);

int main(int argc, char *argv[]) {
    program_name = argv[0];
    bf_options options = parse_arguments(argc, argv);

    /* Check if a file has been provided. */
    if (options.filename == NULL) {
        repl(&options);
    } else {
        run_file(&options);
    }

    return 0;
}


static uint8_t* create_universe(bf_options *options) {
    uint8_t* universe =
        calloc(options->minimum_universe_size, sizeof(uint8_t));
    if (universe == NULL) {
        fprintf(stderr, "%s: could not create universe (%lu bytes): ",
                program_name, options->minimum_universe_size);
        perror(NULL);
        exit(-1);
    }

    return universe;
}

static struct bf_runtime_context normal_context(uint8_t *universe) {
    return ((struct bf_runtime_context) {
            .universe = universe,
            .output_byte = bf_runtime_output_byte,
            .input_byte = bf_runtime_input_byte
    });
}

static void prompt(const char *herp) {
    printf("%s ", herp);
    fflush(stdout);
}


static void repl(bf_options *options) {
    char line[REPL_LINE_LENGTH];

    /* Warn if stdin doesn't appear to be a terminal. */
    if (!isatty(fileno(stdin))) {
        fprintf(stderr,
                "%s: warning: input is not from a terminal\n",
                program_name);
    }

    printf("brainmuk repl " BF_VERSION "\n"
           "Press ctrl+d to exit\n");

    /* Prepare universe and executable space. */
    const size_t exec_mem_size = sysconf(_SC_PAGESIZE);
    uint8_t *universe = create_universe(options);
    uint8_t *exec_mem = allocate_executable_space(exec_mem_size);

    do {
        prompt("#%@!>");

        /* Read. */
        if (fgets(line, REPL_LINE_LENGTH, stdin) == NULL) {
            break;
        }

        /* Eval. */
        bf_compile_result result = bf_compile(line, exec_mem);

        if (result.status != BF_COMPILE_SUCCESS) {
            fprintf(stderr, "compile error (check brackets?)\n");
            continue;
        }

        /* Run! */
        result.program(normal_context(universe));

        /* (The program should print stuff itself... */
    } while (!feof(stdin));

    free(universe);
}

static void run_program(program_t program, bf_options *options) {
    /* Allocate the ENTIRE UNIVERSE and run. */
    uint8_t *universe = calloc(options->minimum_universe_size, sizeof(uint8_t));

    program((struct bf_runtime_context) {
            .universe = universe,
            .output_byte = bf_runtime_output_byte,
            .input_byte = bf_runtime_input_byte
    });

    free(universe);
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
    } else {
        fprintf(stderr, "%s: %s: compilation failed!\n",
                program_name, options->filename);
    }

    free_executable_space(exec_mem, exec_mem_size);
    exit(compilation.status);
}
