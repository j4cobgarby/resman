// vim: fdm=marker
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"
#include "resman.h"

int subcmd_run(int argc, char **argv) { /*{{{*/
    struct args_run args = {NULL, NULL, 0, 0};

    int soc;
    sigset_t sigset;
    int sig;

    job_descriptor job;
    ipc_request req;
    char ser_buff[sizeof(req)];
    int ser_len;

    argp_parse(&argp_run, argc - 1, argv + 1, 0, 0, (void *)&args);

    if (!args.cmd) {
        fprintf(stderr, "[error] No command found after parsing.\n");
        return -1;
    }

    if (args.verbose) {
        printf("Command (argc=%d): ", args.n_cmd_args);

        for (char **cmd_part = args.cmd; *cmd_part; cmd_part++) {
            printf("%s ", *cmd_part);
        }
        printf("\n");

        if (args.msg) {
            printf("Message: %s\n", args.msg);
        } else {
            printf("No message given.\n");
        }
    }

    if (strlen(args.msg) > JOB_MSG_LEN) {
        printf(
            "You have specified a message longer than the maximum of %d, so it "
            "will be truncated.\n",
            JOB_MSG_LEN);
    }

    job.uid = getuid();
    job.t_submitted = time(NULL);
    job.cmd.pid = getpid();
    job.job_type = JOB_CMD;

    if (args.msg) {
        strncpy(job.msg, args.msg, JOB_MSG_LEN);
    } else {
        strcpy(job.msg, "(No message)");
    }

    req.req_type = IPCREQ_JOB;
    req.job = job;

    ser_len = sizeof(req);
    memcpy(ser_buff, &req, sizeof(req));

    if ((soc = connect_to_server(socket_addr)) < 0) {
        fprintf(stderr, "[error] Failed to connect to daemon.\n");
        return -1;
    }

    if (send(soc, ser_buff, ser_len, 0) < 0) {
        perror("[error] Failed send()'ing request.\n");
        return -1;
    }

    if (sigemptyset(&sigset) < 0) {
        perror("sigemptyset");
        return -1;
    }

    if (sigaddset(&sigset, SIGUSR1) < 0) {
        perror("sigaddset");
        return -1;
    }

    if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
        perror("sigprocmask");
        return -1;
    }

    if (args.verbose) printf("\033[0;36m[info] Waiting for signal.\n\033[0m");

    if (sigwait(&sigset, &sig) != 0) {
        fprintf(stderr, "[error] Failed waiting for signal.\n");
        return -1;
    }

    if (args.verbose) printf("\033[0;32m[info] Got signal, running!\033[0m\n");

    execvp(args.cmd[0], args.cmd);

    fprintf(stderr,
            "[error] Failed to execute your command! Double check the "
            "executable name/permissions.\n");
    return -1;
} /*}}}*/

int subcmd_time(int argc, char **argv) { /*{{{*/
    struct args_time args = {NULL, -1, 0};
    job_descriptor job = {0};
    ipc_request req;

    int soc;

    char ser_buff[sizeof(req)];
    int ser_len;

    argp_parse(&argp_time, argc - 1, argv + 1, 0, 0, (void *)&args);

    if (args.verbose) {
        if (args.seconds <= 0) {
            fprintf(stderr, "No duration was given.\n");
            return -1;
        } else {
            printf("Duration: %d seconds\n", args.seconds);
        }

        if (args.msg) {
            printf("Message: %s\n", args.msg);
        } else {
            printf("No message given.\n");
        }
    }

    job.uid = getuid();
    job.t_submitted = time(NULL);
    job.job_type = JOB_TIMESLOT;
    job.timeslot.secs = args.seconds;

    if (args.msg) {
        strncpy(job.msg, args.msg, JOB_MSG_LEN);
    } else {
        strcpy(job.msg, "(No message)");
    }

    req.req_type = IPCREQ_JOB;
    req.job = job;

    ser_len = sizeof(req);
    memcpy(ser_buff, &req, sizeof(req));

    if ((soc = connect_to_server(socket_addr)) < 0) {
        fprintf(stderr, "[error] Failed to connect to daemon.\n");
        return -1;
    }

    if (send(soc, ser_buff, ser_len, 0) < 0) {
        perror("send");
        return -1;
    }

    return 0;
} /*}}}*/

int subcmd_queue(int argc UNUSED, char **argv UNUSED) { /*{{{*/
    info_request info = {0};
    ipc_request req;
    char ser_buff[sizeof(req)];
    int soc;

    info.n_view = 5;
    req.req_type = IPCREQ_VIEW_QUEUE;
    req.info = info;

    memcpy(ser_buff, &req, sizeof(req));

    if ((soc = connect_to_server(socket_addr)) < 0) {
        fprintf(stderr, "[error] Failed to connect to daemon.\n");
        return -1;
    }

    if (send(soc, ser_buff, sizeof(req), 0) < 0) {
        perror("send");
        return -1;
    }

    return 0;
} /*}}}*/
