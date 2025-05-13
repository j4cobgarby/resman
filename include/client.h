// vim: fdm=marker
#ifndef CLIENT_H
#define CLIENT_H
#include <argp.h>
#include "resman.h"

#define CLIENT_VER_STRING "0.0"

/*{{{ Command-line argument parsing defs */
int subcmd_run(int argc, char **argv);
int subcmd_time(int argc, char **argv);
int subcmd_check(int argc, char **argv);
int subcmd_dequeue(int argc, char **argv);

error_t parser_run(int key, char *arg, struct argp_state *state);
error_t parser_time(int key, char *arg, struct argp_state *state);
error_t parser_check(int key, char *arg, struct argp_state *state);
error_t parser_dequeue(int key, char *arg, struct argp_state *state);

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

static struct argp_option options_run[] = {
    {"msg",     'm', "MESSAGE", 0, "Description of your job.", 0},
    {"verbose", 'V', 0,         0, "Give verbose output.",     0},
    {0,         0,   0,         0, 0,                          0},
};

static struct argp argp_run = {
    options_run, &parser_run, "COMMAND", "Submits a job to resmand.",
    NULL,        NULL,        NULL};

static struct argp_option options_time[] = {
    {"msg",     'm', "MESSAGE", 0, "Explanation for your reservation.", 0},
    {"verbose", 'V', 0,         0, "Give verbose output.",              0},
    {0,         0,   0,         0, 0,                                   0},
};

static struct argp argp_time = {
    options_time, &parser_time,
    "DURATION",   "Reserves the server for some time.",
    NULL,         NULL,
    NULL};

static struct argp_option options_check[] = {
    {"silent",  's', 0, 0, "Don't print anything, just return status value.", 0},
    {"count",   'n', "COUNT", 0, "How many queued jobs to view.", 0},
    {"verbose", 'V', 0, 0, "Give verbose output.",          0},
    {0,         0,   0, 0, 0,                               0},
};

static struct argp argp_check = {
    options_check, &parser_check, NULL, "View running and queued jobs.",
    NULL,          NULL,          NULL,
};

static struct argp_option options_dequeue[] = {
    {"verbose", 'V', 0, 0, "Give verbose output.", 0},
    {0,         0,   0, 0, 0,                      0},
};

static struct argp argp_dequeue = {
    options_dequeue,
    &parser_dequeue,
    "JOB_ID",
    "Dequeue a job.",
    NULL,
    NULL,
    NULL,
};

/*}}}*/

#endif /* CLIENT_H */
