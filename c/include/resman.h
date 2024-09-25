#ifndef RESMAN_H
#define RESMAN_H

#include <stdint.h>
#include <sys/types.h> /* uid_t, pid_t */
#include <time.h> /* time() */
#include <stdio.h> /* fopen(), printf() */

enum job_type {
    JOB_CMD,
    JOB_TIMESLOT,
};

typedef struct job_descriptor {
    /* Next job in linked list */
    struct job_descriptor *next;
    uid_t uid;

    time_t t_submitted;
    time_t t_started;
    time_t t_ended;

    char *msg;

    enum job_type req_type;
    union {
	struct {
	    /* PID to signal when it's time to start job */
	    pid_t pid;
	} cmd;
	struct {
	    long secs;
	} timeslot;
    };
} job_descriptor;

typedef struct {
    /* On request: max num jobs to query from queue
     * On response: how many total jobs in queue (maybe less than returned) */
    int n_jobs;
    /* First job in response linked list */
    job_descriptor *first_job;
} info_request;

void free_job_descriptor(job_descriptor *job);

int readcsv_job(FILE *csv, job_descriptor *job);
int writecsv_job(FILE *csv, job_descriptor *job);

int deserialise_job(const char *buf, job_descriptor *job);
int serialise_job(char *buf, size_t len, job_descriptor *job);

#endif
