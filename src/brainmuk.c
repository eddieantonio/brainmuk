#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

#include <brainmuk.h>

#include <bf_alloc.h>
#include <bf_compile.h>
#include <bf_slurp.h>
#include <bf_version.h>

__attribute__((noreturn))
static void usage_error(const char *program_name);

static void bf_runtime_output_byte(uint8_t byte);
static uint8_t bf_runtime_input_byte();

int brainmuk(int argc, char *argv[]) {
    bf_options options = parse_arguments(argc, argv);

    /* Check if a file has been provided. */
    if (options.filename == NULL) {
        fprintf(stderr, "%s: Must provide one filename\n", argv[0]);
        usage_error(argv[0]);
    }

    char *contents = slurp(options.filename);

    /* Try to open the file... */
    if (contents == NULL) {
        fprintf(stderr, "%s: Could not open '%s': ",
                argv[0], options.filename);
        perror(NULL);
        exit(-1);
    }

    /* Get space for the stuff. */
    uint8_t *exec_mem = allocate_executable_space(sysconf(_SC_PAGESIZE));

    /* Compile and forget the source. */
    bf_compile_result compilation = bf_compile(contents, exec_mem);
    unslurp(contents);

    if (compilation.status == BF_COMPILE_SUCCESS) {
        /* Allocate the UNIVERSE and run. */
        uint8_t *universe = calloc(options.minimum_universe_size, sizeof(uint8_t));

        if (universe == NULL) {
            fprintf(stderr, "%s: could not create universe (%lu bytes): ",
                    argv[0], options.minimum_universe_size);
            perror(NULL);
            exit(-1);
        }

        compilation.program((struct bf_runtime_context) {
            .universe = universe,
            .output_byte = bf_runtime_output_byte,
            .input_byte = bf_runtime_input_byte
        });
        free(universe);
    } else {
        fprintf(stderr, "%s: %s: compilation failed!\n",
                argv[0], options.filename);
        exit(compilation.status);
    }

    return 0;
}

static void usage(const char* program_name, FILE *stream) {
    fprintf(stream,
        "Usage:\t%s [-m SIZE] file\n"
        "\t%s [--help|--version]\n",
        program_name, program_name);
}

__attribute__((noreturn))
static void usage_error(const char *program_name) {
    usage(program_name, stderr);
    exit(0);
}

static void version(const char* program_name) {
    printf("%s " BF_VERSION "\n", program_name);
}

#define INVALID_SIZE    0

/* Returns the next non-digit character or NULL. */
static const char* parse_factor(const char *str, unsigned long *factor) {
    char *endptr;
    long int value = strtol(str, &endptr, 10);

    /* Invalid when endpointer didn't move or value isn't a natural number. */
    if (endptr <= str || value < 1) {
        return NULL;
    }

    *factor = value;
    return endptr;
}

static size_t parse_size(const char *str) {
    unsigned long factor;
    unsigned int unit = 1024 * 1024;
    const char *tail;

    if ((tail = parse_factor(str, &factor)) == NULL) {
        return INVALID_SIZE;
    }

    switch (*tail) {
        case '\0': /* no unit. */
        case ' ':
        case '\t':
            break;

        case 'k':
        case 'K':
            unit = 1024;
            break;

        case 'm':
        case 'M':
            unit = 1024 * 1024;
            break;

        case 'g':
        case 'G':
            unit = 1024 * 1024 * 1024;
            break;

        default:
            return INVALID_SIZE;
    }

    return factor * unit;
}

bf_options parse_arguments(int argc, char **argv) {
    bf_options parameters = {
        .minimum_universe_size = 640 * 1024, /* ought to be enough for anybody. */
        .filename = NULL
    };

    static const struct option longopts[] = {
        {
            .name = "help",
            .has_arg = no_argument,
            .flag = NULL,
            .val = 'h',
        },
        {
            .name = "minimum-universe",
            .has_arg = required_argument,
            .flag = NULL,
            .val = 'm',
        },
        {
            .name = "version",
            .has_arg = no_argument,
            .flag = NULL,
            .val = 'v',
        },
        { NULL, 0, NULL, 0 }
    };
    int option = -1;


    while ((option = getopt_long(argc, argv, "hm:v", longopts, NULL)) != -1) {
        switch (option) {
            case 'h': /* --help */
                usage(argv[0], stdout);
                break;

            case 'm': /* --minimum-universe */
                parameters.minimum_universe_size = parse_size(optarg);

                if (parameters.minimum_universe_size == INVALID_SIZE) {
                    fprintf(stderr, "Invalid size: %s\n", optarg);
                    usage_error(argv[0]);
                }

                break;

            case 'v': /* --version */
                version(argv[0]);
                exit(0);
                break;

            default:
                /* getopt_long will print unrecognized argument message. */
                usage_error(argv[0]);
        }
    }

    /* If we have arguments left-over, let it be the filename. */
    if (optind < argc) {
        parameters.filename = argv[optind];
    }

    /* RESET GLOBAL STATE! Yeah, getopt_long() works with globals... */
    optind = 0;

    return parameters;
}

/**
 * A void wrapper for putchar.
 */
static void bf_runtime_output_byte(uint8_t byte) {
    putchar(byte);
}

/**
 * @return  the character; if the stream has reached EOF, returns 0xFF.
 *          0xFF is a good value to use because it's relatively easy to
 *          compare to (simply add one), and it's an invalid UTF-8 byte.
 */
static uint8_t bf_runtime_input_byte() {
    int c = getchar();
    if (c == EOF) {
        return 0xFF;
    }
    return c;
}
