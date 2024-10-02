// vim: fdm=marker
#include "client.h"

void print_subcmds(char *prog) {/*{{{*/
    fprintf(stderr, "Usage: %s [subcommand] <options>\n", prog);
    fprintf(stderr, "Valid subcommands: run, time, queue\n");
}/*}}}*/

error_t parser_run(int key, char *arg, struct argp_state *state){/*{{{*/
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
                return ARGP_ERR_UNKNOWN;
            }
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}/*}}}*/

error_t parser_time(int key, char *arg, struct argp_state *state){/*{{{*/
    struct args_time *args = (struct args_time *)state->input;
    switch (key) {
        case 'V':
            printf("[info] Verbose mode enabled.\n");
            args->verbose = 1;
            break;

    }
}/*}}}*/

error_t parser_queue(int key, char *arg, struct argp_state *state){/*{{{*/
    return 0;
}/*}}}*/
