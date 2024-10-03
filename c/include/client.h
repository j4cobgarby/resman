// vim: fdm=marker
#ifndef CLIENT_H
#define CLIENT_H
#include <argp.h>

#define CLIENT_VER_STRING "0.0"

/*{{{ Command-line argument parsing defs */
int subcmd_run(int argc, char **argv);
int subcmd_time(int argc, char **argv);
int subcmd_queue(int argc, char **argv);

error_t parser_run(int key, char *arg, struct argp_state *state);
error_t parser_time(int key, char *arg, struct argp_state *state);
error_t parser_queue(int key, char *arg, struct argp_state *state);

int connect_to_server(const char *addr);
void print_subcmds(char *prog);

struct args_run {
    char *msg;
    char **cmd;
    int n_cmd_args;
    int verbose;
};

struct args_time {
    char *msg;
    unsigned int seconds;
    int verbose;
};

struct args_queue {
    int n;
    int verbose;
};

static struct argp_option options_run[] = {
    {"msg",     'm', "MESSAGE", 0, "Description of your job.", 0},
    {"verbose", 'V', 0,         0, "Give verbose output.",     0},
    {0},
};

static struct argp argp_run = {
    options_run, &parser_run, "COMMAND", "Submits a job to resmand.",
    NULL, NULL, NULL
};

static struct argp_option options_time[] = {
    {"msg",     'm', "MESSAGE", 0, "Explanation for your reservation.", 0},
    {"verbose", 'V', 0,         0, "Give verbose output.",              0},
    {0},
};

static struct argp argp_time = {
    options_time, &parser_time, "DURATION", "Reserves the server for some time.",
    NULL, NULL, NULL
};

/*}}}*/

#endif /* CLIENT_H */
