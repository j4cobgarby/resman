// vim: fdm=marker
#include <pwd.h>     // struct passwd
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"
#include "resman.h"

static int send_ipc_request(int soc, const ipc_request *req) {/*{{{*/
    if (!req) {
        fprintf(stderr, "[bug]");
        return -1;
    }
    return send(soc, req, sizeof(ipc_request), 0);
}/*}}}*/

static int get_status(int soc, status_response *stat) {/*{{{*/
    if (!stat) return -1;
    if (recv(soc, stat, sizeof(status_response), 0) < 0) {
        return -1;
    }
    printf("Read status: %d\n", stat->status);
    return 0;
}/*}}}*/

int subcmd_run(int argc, char **argv) { /*{{{*/
    struct args_run args = {NULL, NULL, 0, 0};

    int soc;
    sigset_t sigset;
    int sig;

    job_descriptor job;
    ipc_request req;

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

    if (args.msg && strlen(args.msg) > JOB_MSG_LEN) {
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

    if ((soc = connect_to_server(socket_addr)) < 0) {
        fprintf(stderr, "[error] Failed to connect to daemon.\n");
        return -1;
    }

    if (send_ipc_request(soc, &req) < 0) {
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

    if (args.verbose) {
        printf("\033[0;32m[info] Got signal, running!\033[0m\n");
        printf("Command:\n");
        for (int i = 0; args.cmd[i]; i++) {
            printf("'%s'\n", args.cmd[i]);
        }
    }

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

    if ((soc = connect_to_server(socket_addr)) < 0) {
        fprintf(stderr, "[error] Failed to connect to daemon.\n");
        return -1;
    }

    if (send_ipc_request(soc, &req) < 0) {
        perror("send");
        return -1;
    }

    return 0;
} /*}}}*/

int subcmd_check(int argc UNUSED, char **argv UNUSED) { /*{{{*/
    struct args_check args = {.n = 5};
    info_request info = {0};
    ipc_request req;
    queue_info_response_header *resp_header;
    char *resp_buf;
    unsigned int resp_maxlen;
    int soc;

    int ret = argp_parse(&argp_check, argc - 1, argv + 1, 0, 0, (void *)&args);
    if (ret != 0) {
        exit(-1);
    }

    info.n_view = args.n;
    req.req_type = IPCREQ_VIEW_QUEUE;
    req.info = info;

    if ((soc = connect_to_server(socket_addr)) < 0) {
        fprintf(stderr, "[error] Failed to connect to daemon.\n");
        return -1;
    }

    if (send_ipc_request(soc, &req) < 0) {
        perror("send");
        return -1;
    }

    resp_maxlen = sizeof(*resp_header) + info.n_view * sizeof(job_descriptor);
    resp_buf = malloc(resp_maxlen);

    if (!resp_buf) {
        perror("malloc");
        return -1;
    }

    if (recv(soc, resp_buf, resp_maxlen, 0) < 0) {
        perror("recv");
        return -1;
    }

    job_descriptor *jobs =
        (job_descriptor *)(resp_buf + sizeof(queue_info_response_header));
    resp_header = (queue_info_response_header *)resp_buf;
    if (resp_header->currently_running) {
        printf(CLR_RED "** A job is currently running **\n" CLR_END);
        printf("%d other jobs are queued.\n", resp_header->total_count - 1);
    } else {
        printf("%d jobs are queued, none are running.\n",
               resp_header->total_count);
    }

    const char *head_fmt = " %4s | %-8s | %-8s | %-19s | %s\n";
    const char *tab_fmt = "%4d | %-8s | %-8s | %-19s | %s\n";

    printf(head_fmt, "uuid", "type", "user", "time submitted", "message");
    printf(head_fmt, "---", "---", "---", "---", "---");

    char time_buf[32];

    for (int i = 0; i < (int)resp_header->resp_count; i++) {
        struct passwd *pwd = getpwuid(jobs[i].uid);
        strftime(time_buf, sizeof(time_buf), "%a %e %b %T",
                 localtime(&jobs[i].t_submitted));

        if (resp_header->currently_running && i == 0) printf(CLR_BLUE ">");
        else printf(" ");

        printf(tab_fmt, jobs[i].job_uuid, jobtype_lbl[jobs[i].job_type],
               pwd ? pwd->pw_name : "---", time_buf, jobs[i].msg);

        if (resp_header->currently_running && i == 0) printf("\033[0m");
    }

    return 0;
} /*}}}*/

int subcmd_dequeue(int argc, char **argv) { /*{{{*/
    struct args_dequeue args = {.job_id = -1, .verbose = 0};
    ipc_request req;
    status_response stat;
    int soc;

    int ret =
        argp_parse(&argp_dequeue, argc - 1, argv + 1, 0, 0, (void *)&args);
    if (ret != 0) {
        if (ret == EINVAL) {
            fprintf(stderr, "Invalid job ID.\n");
            exit(EINVAL);
        }
        exit(-1);
    }

    if (args.job_id > UUID_MAX || args.job_id < 0) {
        fprintf(stderr, "Invalid job ID.\n");
        return -1;
    }

    req.req_type = IPCREQ_DEQUEUE;
    req.deq.job_uuid = (uuid_t)args.job_id;

    if ((soc = connect_to_server(socket_addr)) < 0) {
        fprintf(stderr, "[error] Failed to connect to daemon.\n");
        return -1;
    }

    if (send_ipc_request(soc, &req) < 0) {
        perror("send");
        return -1;
    }

    if (get_status(soc, &stat) < 0) {
        perror("get_status");
        return -1;
    }

    if (stat.status == STATUS_OK) {
        printf("Succesfully dequeued job %d\n", args.job_id);
    } else {
        printf("Dequeue failed. Status = %d\n", stat.status);
        return -1;
    }

    return 0;
} /*}}}*/
