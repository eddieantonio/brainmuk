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

static void version(const char* program_name) {
    printf("%s 0.1.0\n", program_name);
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
                exit(0);
                break;
            case 'm': /* --minimum-universe */
                parameters.minimum_universe_size = atoi(optarg) * 1024 * 1024;
                break;
            case 'v': /* --version */
                version(argv[0]);
                exit(0);
                break;
            default:
                /* getopt_long will print unrecognized argument message. */
                usage(argv[0], stderr);
                exit(-1);
        }
    }

    return parameters;
}
