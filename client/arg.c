// vim: fdm=marker
#include <argp.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"

void print_subcmds(char *prog) { /*{{{*/
    fprintf(stderr, "Usage: %s SUBCOMMAND [OPTION...]\n", prog);
    fprintf(stderr, "Valid subcommands: [r]un, [t]ime, [c]heck, [d]equeue\n");
} /*}}}*/

static unsigned int parse_duration(const char *s) { /*{{{*/
    unsigned int secs = 0;
    unsigned long acc;
    char *endptr;
    const char *s_end = s + strlen(s);

    /* Up to three stages, one for each unit (h,m,s) */
    for (int stage = 0; stage < 3; stage++) {
        errno = 0;
        acc = strtoul(s, &endptr, 10);

        if (errno != 0) {
            /* Failed to parse */
            perror("strtoul");
            return -1;
        } else if (endptr == s_end) {
            /* Got to the end, so just return what we have */
            return secs;
        } else if (endptr == s) {
            /* Couldn't parse any further, due to invalid string */
            return -1;
        } else {
            switch (*endptr) {
                case 'h': /* Hours */
                    if (stage > 0) return -1;
                    secs += acc * 3600;
                    break;
                case 'm': /* Minutes */
                    if (stage > 1) return -1;
                    secs += acc * 60;
                    break;
                case 's': /* Seconds */
                    secs += acc;
                    return secs;
                default:
                    return -1;
            }
        }

        /* Skip to after unit */
        s = endptr + 1;
    }

    return secs;
} /*}}}*/

error_t parser_run(int key, char *arg, struct argp_state *state) { /*{{{*/
    struct args_run *args = (struct args_run *)state->input;

    switch (key) {
        case 'm':
            args->msg = arg;
            break;
        case 'V':
            printf("[info] Verbose mode enabled.\n");
            args->verbose = 1;
            break;
        case ARGP_KEY_ARG:
            // Collect all trailing non-opt arguments (so, the command they
            // want to run), by just pointing to the start of the part of the
            // array where those args are.
            args->cmd = &state->argv[state->next - 1];
            args->n_cmd_args = state->argc - state->next + 1;
            // Then stop parsing here.
            state->next = state->argc;
            break;
        case ARGP_KEY_END:
            if (state->arg_num == 0) {
                argp_usage(state);
            }
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
} /*}}}*/

error_t parser_time(int key, char *arg, struct argp_state *state) { /*{{{*/
    struct args_time *args = (struct args_time *)state->input;

    switch (key) {
        case 'V':
            printf("[info] Verbose mode enabled.\n");
            args->verbose = 1;
            break;
        case 'm':
            args->msg = arg;
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num == 0) {
                int dur = parse_duration(arg);
                if (dur == -1) {
                    argp_error(state, "Invalid duration format: '%s'n", arg);
                }
                args->seconds = dur;
                break;
            } else {
                argp_usage(state);
            }
        case ARGP_KEY_END:
            if (state->arg_num == 0) {
                argp_usage(state);
            }
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
} /*}}}*/

error_t parser_check(int key, char *arg, struct argp_state *state) { /*{{{*/
    struct args_check *args = (struct args_check *)state->input;
    char *end;

    switch (key) {
        case 'V':
            printf("[info] Verbose mode enabled.\n");
            args->verbose = 1;
            break;
        case 's':
            args->silence = 1;
            break;
        case 'n':
            errno = 0;
            args->n = strtol(arg, &end, 10);
            if (errno == ERANGE || end == arg) {
                argp_error(state, "Invalid count: '%s'", arg);
            }
            break;
        case ARGP_KEY_ARG:
        case ARGP_KEY_END:
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
} /*}}}*/

error_t parser_dequeue(int key, char *arg, struct argp_state *state) { /*{{{*/
    struct args_dequeue *args = (struct args_dequeue *)state->input;
    char *end;

    switch (key) {
        case 'V':
            printf("[info] Verbose mode enabled.\n");
            args->verbose = 1;
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num == 0) {
                errno = 0;
                args->job_id = strtoul(arg, &end, 10);
                if (errno == ERANGE || end == arg) {
                    argp_error(state, "Invalid job id format: '%s'", arg);
                }
            } else {
                argp_usage(state);
            }
            break;
        case ARGP_KEY_END:
            if (state->arg_num == 0) {
                argp_usage(state);
            }
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
} /*}}}*/
