// vim: fdm=marker
#include <pwd.h>  // struct passwd
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"
#include "resman.h"

const char* jobtype_lbl[] = {
    "Command",
    "Time",
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
    options_time,
    &parser_time,
    "DURATION",
    "Reserves the server for the given amount of time, specified using "
    "suffixes 's', 'm', or 'h' to specify seconds, minutes, or hours. These "
    "can be combined (e.g. 3h10m5s). A number without suffix is treated as "
    "seconds (e.g. a duration of 120 will reserve the server for two minutes).",
    NULL,
    NULL,
    NULL};

static struct argp_option options_check[] = {
    {"silent",  's', 0,       0, "Don't print anything, just return status value.", 0},
    {"count",   'n', "COUNT", 0, "How many queued jobs to view.",                   0},
    {"verbose", 'V', 0,       0, "Give verbose output.",                            0},
    {0,         0,   0,       0, 0,                                                 0},
};

static struct argp argp_check = {
    options_check, &parser_check, NULL, "View running and queued jobs.",
    NULL,          NULL,          NULL,
};

static struct argp_option options_dequeue[] = {
    {"force",   'f', 0, 0, "Even if target job belongs to different user.", 0},
    {"verbose", 'V', 0, 0, "Give verbose output.",                          0},
    {0,         0,   0, 0, 0,                                               0},
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

static struct argp_option options_release[] = {
    {"force",   'f', 0, 0, "Even if current job belongs to different user.", 0},
    {"verbose", 'V', 0, 0, "Give verbose output.",                           0},
    {0,         0,   0, 0, 0,                                                0},
};

static struct argp argp_release = {
    options_release,
    &parser_release,
    NULL,
    "Release current job's lock. Does not terminate job.",
    NULL,
    NULL,
    NULL,
};

static ssize_t send_ipc_request(const int soc, const ipc_request* req) { /*{{{*/
    if (!req) {
        fprintf(stderr, "[bug] send_ipc_request received nullptr");
        return -1;
    }
    return send(soc, req, sizeof(ipc_request), 0);
} /*}}}*/

static int get_status(const int soc, status_response* stat) { /*{{{*/
    if (!stat) return -1;
    if (recv(soc, stat, sizeof(status_response), 0) < 0) {
        return -1;
    }
    return 0;
} /*}}}*/

int subcmd_run(const int argc, char** argv) { /*{{{*/
    struct args_run args = {NULL, NULL, 0, 0};

    int soc;
    sigset_t sigset;
    int sig;

    job_descriptor job;
    ipc_request req;
    status_response resp;

    argp_parse(&argp_run, argc - 1, argv + 1, 0, 0, (void*)&args);

    if (!args.cmd) {
        fprintf(stderr, "[error] No command found after parsing.\n");
        return -1;
    }

    if (args.verbose) {
        printf("Command (argc=%d): ", args.n_cmd_args);

        for (char** cmd_part = args.cmd; *cmd_part; cmd_part++) {
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

    job.t_submitted = time(NULL);
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

    if (send_ipc_request(soc, &req) < 0) {
        perror("[error] Failed send()'ing request.\n");
        return -1;
    }

    if (get_status(soc, &resp) < 0) {
        perror("get_status");
        return -1;
    }

    if (resp.status != STATUS_OK) {
        fprintf(stderr, "Failed to enqueue job.\n");
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

int subcmd_time(const int argc, char** argv) { /*{{{*/
    struct args_time args = {NULL, -1, 0};
    job_descriptor job = {0};
    ipc_request req;
    status_response resp;

    int soc;

    argp_parse(&argp_time, argc - 1, argv + 1, 0, 0, (void*)&args);

    if (args.seconds <= 0) {
        fprintf(stderr, "[error] Invalid duration, must be > 0 seconds.\n");
        return -1;
    }

    if (args.verbose) {
        printf("Duration: %d seconds\n", args.seconds);
        if (args.msg) {
            printf("Message: %s\n", args.msg);
        } else {
            printf("No message given.\n");
        }
    }

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

    if (get_status(soc, &resp) < 0) {
        perror("get_status");
        return -1;
    }

    if (resp.status != STATUS_OK) {
        fprintf(stderr,
                "[error] Could not reserve timeslot. Perhaps the server "
                "is already in use?\n");
        return -1;
    }

    return 0;
} /*}}}*/

int subcmd_check(int argc UNUSED, char** argv UNUSED) { /*{{{*/
    struct args_check args = {.n = 5};
    info_request info = {0};
    ipc_request req;
    queue_info_response_header* resp_header;
    char* resp_buf;
    unsigned int resp_maxlen;
    int soc;

    const int ret = argp_parse(&argp_check, argc - 1, argv + 1, 0, 0, &args);
    if (ret != 0) {
        exit(-1);
    }

    info.n_view = args.n;
    req.req_type = IPCREQ_VIEW_QUEUE;
    req.info = info;

    if (info.n_view <= 0) {
        fprintf(stderr, "[error] n must be > 0\n");
        return -1;
    }

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
        free(resp_buf);
        return -1;
    }

    job_descriptor* jobs =
        (job_descriptor*)(resp_buf + sizeof(queue_info_response_header));
    resp_header = (queue_info_response_header*)resp_buf;
    if (!args.silence) {
        if (resp_header->currently_running) {
            printf(CLR_RED "** A job is currently running **\n" CLR_END);
            printf("%d other jobs are queued.\n", resp_header->total_count - 1);
            char s_time[32];
            strftime(s_time, sizeof(s_time), "%a %e %b %T",
                     localtime(&jobs[0].t_started));
            printf("Current job started .. " CLR_BLUE "%s\n" CLR_END, s_time);

            strftime(s_time, sizeof(s_time), "%a %e %b %T",
                     localtime(&jobs[0].timeslot.t_end));
            if (jobs[0].job_type == JOB_TIMESLOT) {
                printf("It will end at ....... " CLR_BLUE "%s\n" CLR_END,
                       s_time);
            }
        } else {
            printf("%d jobs are queued, none are running.\n",
                   resp_header->total_count);
        }

        if (resp_header->resp_count > 0) {
            const char* head_fmt = " %4s | %-8s | %-8s | %-19s | %s\n";
            const char* tab_fmt = "%4d | %-8s | %-8s | %-19s | %s\n";

            printf(head_fmt, "uuid", "type", "user", "time submitted",
                   "message");
            printf(head_fmt, "---", "---", "---", "---", "---");

            char time_buf[32];

            for (int i = 0; i < (int)resp_header->resp_count; i++) {
                const struct passwd* pwd = getpwuid(jobs[i].uid);
                strftime(time_buf, sizeof(time_buf), "%a %e %b %T",
                         localtime(&jobs[i].t_submitted));

                if (resp_header->currently_running && i == 0)
                    printf(CLR_BLUE ">");
                else
                    printf(" ");

                printf(tab_fmt, jobs[i].job_uuid, jobtype_lbl[jobs[i].job_type],
                       pwd ? pwd->pw_name : "---", time_buf, jobs[i].msg);

                if (resp_header->currently_running && i == 0) printf(CLR_END);
            }
        }
    }

    const int running = resp_header->currently_running;
    free(resp_buf);
    return running;
} /*}}}*/

int subcmd_dequeue(const int argc, char** argv) { /*{{{*/
    struct args_dequeue args = {.job_id = -1, .force = 0, .verbose = 0};
    ipc_request req;
    status_response stat;
    int soc;

    const int ret =
        argp_parse(&argp_dequeue, argc - 1, argv + 1, 0, 0, (void*)&args);
    if (ret != 0) {
        if (ret == EINVAL) {
            fprintf(stderr, "Invalid job ID\n");
            exit(EINVAL);
        }
        exit(-1);
    }

    if (args.job_id > UUID_MAX || args.job_id < 0) {
        fprintf(stderr, "Invalid job ID\n");
        return -1;
    }

    req.req_type = IPCREQ_DEQUEUE;
    req.deq.job_uuid = args.job_id;
    req.deq.force = args.force;

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

    switch (stat.status) {
        case STATUS_OK:
            printf("Successfully dequeued job %d\n", args.job_id);
            break;
        case STATUS_DEQ_FAIL_JOB_CURRENTLY_RUNNING:
            fprintf(stderr,
                    "[error] Cannot dequeue currently running job. Consider "
                    "releasing the lock instead.\n");
            break;
        case STATUS_DEQ_FAIL_NO_SUCH_JOB:
            fprintf(
                stderr,
                "[error] Failed to dequeue job %d: job not found in queue\n",
                args.job_id);
            break;
        case STATUS_DEQ_FAIL_NOT_YOUR_JOB:
            fprintf(
                stderr,
                "[error] Failed to dequeue job %d submitted by another user\n",
                args.job_id);
            break;
        default:
            fprintf(stderr, "[error] Dequeue failed (code: %d)\n", stat.status);
            return -1;
    }

    return 0;
} /*}}}*/

int subcmd_release(const int argc, char** argv) {  // {{{
    struct args_release args = {.force = 0, .verbose = 0};
    ipc_request req;
    status_response stat;
    int soc;

    const int ret =
        argp_parse(&argp_release, argc - 1, argv + 1, 0, 0, (void*)&args);
    if (ret != 0) {
        exit(-1);
    }

    req.req_type = IPCREQ_RELEASE;
    req.rel.force = args.force;

    if ((soc = connect_to_server(socket_addr)) < 0) {
        fprintf(stderr, "[error] Failed to connect to daemon\n");
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

    switch (stat.status) {
        case STATUS_OK:
            printf("Successfully released lock\n");
            break;
        case STATUS_REL_OK_SERVER_IDLE:
            printf("Server is not currently locked, no lock to release.\n");
            break;
        case STATUS_REL_FAIL_NOT_YOUR_JOB:
            fprintf(stderr,
                    "[error] Refusing to release lock held by another user, "
                    "consider using --force.\n");
            break;
        default:
            fprintf(stderr, "[error] Release failed (code %d)\n", stat.status);
            return -1;
    }

    return 0;
}  // }}}
