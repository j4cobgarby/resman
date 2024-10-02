// vim: fdm=marker
#include "client.h"
#include "resman.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

int subcmd_run(int argc, char **argv) {/*{{{*/
    struct args_run args = {NULL, NULL, 0, 0};

    int soc;
    sigset_t sigset;
    int sig;

    job_descriptor job;
    char ser_buff[JOB_SER_MAXLEN];
    int ser_len;

    argp_parse(&argp_run, argc-1, argv+1, 0, 0, (void*)&args);

    if (!args.cmd) {
        fprintf(stderr, "[error] No command found after parsing.\n");
        return -1;
    }

    if (args.verbose) {
        printf("Command (n=%d): ", args.n_cmd_args);
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

    job.uid = getuid();
    job.msg = args.msg;
    job.t_submitted = time(NULL);
    job.cmd.pid = getpid();
    job.req_type = JOB_CMD;

    if ((ser_len = serialise_job(ser_buff, JOB_SER_MAXLEN, &job)) < 0) {
        fprintf(stderr, "[error] Failed to serialise job descriptor.\n");
        return -1;
    }

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
    
    if (args.verbose) printf("Waiting for signal.\n");

    if (sigwait(&sigset, &sig) != 0) {
        fprintf(stderr, "[error] Failed waiting for signal.\n");
        return -1;
    }

    execvp(args.cmd[0], args.cmd);

    fprintf(stderr, "[error] Failed to execute your command!\n");
    return -1;
}/*}}}*/

int subcmd_time(int argc, char **argv) {/*{{{*/
    struct args_time args = {NULL, -1, 0};

    argp_parse(&argp_time, argc-1, argv+1, 0, 0, (void*)&args);

    if (args.seconds < 0) {
        fprintf(stderr, "No duration was given.\n");
        return -1;
    }

    if (args.verbose) {
        if (args.msg) {
            printf("Message: %s\n", args.msg);
        } else {
            printf("No message given.\n");
        }
    }

    return 0;
}/*}}}*/

int subcmd_queue(int argc, char **argv) {/*{{{*/
    printf("subcommand queue\n");
    for (int i = 0; i < argc; i++) {
        printf("* arg #%d = %s\n", i, argv[i]);
    }

    return 0;
}/*}}}*/
