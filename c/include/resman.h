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

enum job_type {
    JOB_CMD,
    JOB_TIMESLOT,
};

typedef struct job_descriptor {
    uid_t uid; // User who submitted job
    time_t t_submitted; // Time job was sent to resman
    char *msg; // Explanatory message

    enum job_type req_type;
    union {
        struct {
            pid_t pid; // Job stub PID
        } cmd;
        struct {
            unsigned int secs; // Seconds to reserve
        } timeslot;
    };

    /* Only used by server: */
    time_t t_started; // Time that job begun
    time_t t_ended; // Time that job ended
    struct job_descriptor *next; // Next job in list
} job_descriptor;

typedef struct info_request {
    /* On request: max num jobs to query from queue
     * On response: how many total jobs returned (maybe less than requested) */
    int n_jobs;
    /* First job in response linked list */
    job_descriptor *first_job;
} info_request;

typedef struct queue_info {
    /* How many jobs queued up */
    unsigned int queue_length;
} queue_info;

/*}}}*/

void free_job_descriptor(job_descriptor *job);

int readcsv_job(FILE *csv, job_descriptor *job);
int writecsv_job(FILE *csv, job_descriptor *job);

int deserialise_job(const char *buf, size_t len, job_descriptor *job);
int serialise_job(char *buf, size_t len, job_descriptor *job);

#endif /* RESMAN_H */
