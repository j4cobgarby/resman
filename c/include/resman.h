// vim: fdm=marker
#ifndef RESMAN_H
#define RESMAN_H

#include <stdio.h>     /* fopen(), printf() */
#include <sys/types.h> /* uid_t, pid_t */
#include <time.h>      /* time() */

#define UNUSED __attribute__((unused))

#define CLR_GREEN "\033[0;32m"
#define CLR_RED "\033[0;31m"
#define CLR_BLUE "\033[0;36m"
#define CLR_END "\033[0m"

extern const char *socket_addr;

/*{{{ IPC Structures */

typedef int uuid_t;
#define UUID_MAX 9999

enum job_type {
    JOB_CMD = 0,
    JOB_TIMESLOT,
};

static const char *jobtype_lbl[] = {
    "Command",
    "Time",
};

enum ipc_request_type {
    IPCREQ_JOB,
    IPCREQ_VIEW_QUEUE,
    IPCREQ_DEQUEUE,
};

#define JOB_MSG_LEN 256

typedef struct job_descriptor {
    uid_t uid;           // User who submitted job
    time_t t_submitted;  // Time job was sent to resman
    char msg[JOB_MSG_LEN];
    uuid_t job_uuid;  // Set by the server

    enum job_type job_type;
    union {
        struct {
            pid_t pid;  // Job stub PID
        } cmd;
        struct {
            unsigned int secs;  // Seconds to reserve
        } timeslot;
    };
} job_descriptor;

typedef struct info_request {
    int n_view;
} info_request;

typedef struct dequeue_request {
    uuid_t job_uuid;
} dequeue_request;

typedef struct queue_info_response_header {
    unsigned int resp_count;
    unsigned int total_count;
    int currently_running; /* 1 if currently a job is running */
} queue_info_response_header;

typedef struct status_response {
    enum {
        STATUS_OK,
        STATUS_FAIL,
    } status;
} status_response;

typedef struct ipc_request {
    enum ipc_request_type req_type;
    union {
        job_descriptor job;
        info_request info;
        dequeue_request deq;
    };
} ipc_request;

/*}}}*/

int readcsv_job(FILE *csv, job_descriptor *job);
int writecsv_job(FILE *csv, job_descriptor *job);

#endif /* RESMAN_H */
