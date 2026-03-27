#include "args.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Argument descriptors — edit descriptions here to update the help text.
 * ----------------------------------------------------------------------- */

static const struct {
    const char* name;
    const char* metavar;
    const char* description;
} positional_args[] = {
    { "filename",      "FILE",    "Path to the WAV file to transmit" },
    { "dartt_address", "ADDR",    "DARTT slave destination address (0-255)" },
    { "dartt_index",   "INDEX",   "DARTT 32-bit-aligned index offset" },
};

static const struct {
    const char* flag;
    const char* metavar;
    const char* description;
    const char* default_val;
} named_args[] = {
    { "--baudrate", "BAUD", "Serial baud rate", "921600" },
};

/* ----------------------------------------------------------------------- */

static void print_help(const char* prog)
{
    printf("Usage: %s <filename> <dartt_address> <dartt_index> [--baudrate BAUD] [-h]\n\n",
           prog);

    printf("Positional arguments (required, in order):\n");
    for (size_t i = 0; i < sizeof(positional_args) / sizeof(positional_args[0]); i++) {
        printf("  %-18s  %s\n", positional_args[i].metavar, positional_args[i].description);
    }

    printf("\nOptional arguments:\n");
    for (size_t i = 0; i < sizeof(named_args) / sizeof(named_args[0]); i++) {
        char flag_meta[64];
        snprintf(flag_meta, sizeof(flag_meta), "%s %s", named_args[i].flag, named_args[i].metavar);
        printf("  %-18s  %s (default: %s)\n",
               flag_meta, named_args[i].description, named_args[i].default_val);
    }
    printf("  %-18s  Show this help message and exit\n", "-h, --help");

    exit(0);
}

static long parse_int(const char* str, const char* arg_name)
{
    char* end;
    long val = strtol(str, &end, 0);   /* base 0: auto-detects 0x hex, 0 octal */
    if (*end != '\0') {
        fprintf(stderr, "error: '%s' is not a valid integer for argument '%s'\n",
                str, arg_name);
        exit(1);
    }
    return val;
}

args_t parse_args(int argc, char** argv)
{
    /* Scan for -h/--help before anything else */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
        }
    }

    args_t args = {};
    args.baudrate = 921600;   /* default */

    /* Separate positionals from named flags */
    const char* positionals[8] = {};
    int positional_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--", 2) == 0) {
            /* Named flag: expect a value to follow */
            if (strcmp(argv[i], "--baudrate") == 0) {
                if (i + 1 >= argc) {
                    fprintf(stderr, "error: --baudrate requires a value\n");
                    exit(1);
                }
                args.baudrate = (int)parse_int(argv[++i], "--baudrate");
            } else {
                fprintf(stderr, "error: unknown flag '%s'\n", argv[i]);
                exit(1);
            }
        } else {
            if (positional_count < (int)(sizeof(positionals) / sizeof(positionals[0]))) {
                positionals[positional_count++] = argv[i];
            }
        }
    }

    /* Check required positionals and report all missing ones */
    static const int required_count = 3;
    bool missing = false;
    for (int i = 0; i < required_count; i++) {
        if (i >= positional_count || positionals[i] == nullptr) {
            fprintf(stderr, "error: missing required argument '%s'\n",
                    positional_args[i].name);
            missing = true;
        }
    }
    if (missing) {
        fprintf(stderr, "Run '%s -h' for usage.\n", argv[0]);
        exit(1);
    }

    /* Populate struct */
    args.filename      = positionals[0];
    args.dartt_address = (unsigned char)parse_int(positionals[1], "dartt_address");
    args.dartt_index   = (uint16_t)(parse_int(positionals[2], "dartt_index"));

    return args;
}
