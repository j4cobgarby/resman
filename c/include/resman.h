// vim: fdm=marker
#ifndef RESMAN_H
#define RESMAN_H

#include <stdio.h>     /* fopen(), printf() */
#include <sys/types.h> /* uid_t, pid_t */
#include <time.h>      /* time() */

#define UNUSED __attribute__((unused))

extern const char *socket_addr;

/*{{{ IPC Structures */
/* Size of serialised job, excluding message */
#define JOB_SER_BASELEN 34

/* Max size of message plus rest of fields */
#define JOB_SER_MAXLEN (255 + JOB_SER_BASELEN)

#define JOB_MSG_LEN 256

enum job_type {
    JOB_CMD,
    JOB_TIMESLOT,
};

enum ipc_request_type {
    IPCREQ_JOB,
    IPCREQ_VIEW_QUEUE,
};

typedef struct job_descriptor {
    uid_t uid; // User who submitted job
    time_t t_submitted; // Time job was sent to resman
    char msg[JOB_MSG_LEN];

    enum job_type job_type;
    union {
        struct {
            pid_t pid; // Job stub PID
        } cmd;
        struct {
            unsigned int secs; // Seconds to reserve
        } timeslot;
    };
} job_descriptor;

typedef struct info_request {
    int n_view;
} info_request;

typedef struct queue_info {
    /* How many jobs queued up */
    unsigned int queue_length;
} queue_info;

typedef struct ipc_request {
    enum ipc_request_type req_type;
    union {
        job_descriptor job;
        info_request info;
    };
} ipc_request;

/*}}}*/

void free_job_descriptor(job_descriptor *job);

int readcsv_job(FILE *csv, job_descriptor *job);
int writecsv_job(FILE *csv, job_descriptor *job);

int deserialise_job(const char *buf, size_t len, job_descriptor *job);
int serialise_job(char *buf, size_t len, job_descriptor *job);

#endif /* RESMAN_H */
