#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include <brainmuk.h>


int brainmuk(int argc, char *argv[]) {
    parse_arguments(argc, argv);
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

/* TODO: generate from Makefile or git-describe. */
static void version(const char* program_name) {
    printf("%s 0.1.0\n", program_name);
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
        .minimum_universe_size = 640 * 1024 /* ought to be enough for anybody. */
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
        { 0 }
    };
    int option = -1;

    /* RESET GLOBAL STATE! Yeah, getopt_long() works with globals... */
    optind = 0;

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

    return parameters;
}
