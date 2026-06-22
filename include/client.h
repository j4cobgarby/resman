// vim: fdm=marker
#ifndef CLIENT_H
#define CLIENT_H

#include <argp.h>

#include "resman.h"

#define CLIENT_VER_STRING "0.0"

/* Command-line argument parsing defs */
int subcmd_run(int argc, char **argv);
int subcmd_time(int argc, char **argv);
int subcmd_check(int argc, char **argv);
int subcmd_dequeue(int argc, char **argv);
int subcmd_release(int argc, char **argv);

error_t parser_run(int key, char *arg, struct argp_state *state);
error_t parser_time(int key, char *arg, struct argp_state *state);
error_t parser_check(int key, char *arg, struct argp_state *state);
error_t parser_dequeue(int key, char *arg, struct argp_state *state);
error_t parser_release(int key, char *arg, struct argp_state *state);

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

struct args_check {
    int n;
    int verbose;
    int silence;
};

struct args_dequeue {
    uuid_t job_id;
    int verbose;
};

struct args_release {
    int force;
    int verbose;
};

#endif /* CLIENT_H */
