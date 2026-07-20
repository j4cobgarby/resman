// vim: fdm=marker
#include <stdlib.h>
#include "resman.h"
#include "server.h"

/* Take a pointer to the job at the front of the queue, without removing it.
 * An offset can be specified, if a job further in the queue is needed.
 * Returns NULL if the requested queue position is empty. */
const queued_job *peek_job(const queued_job *q, const int off) { /*{{{*/
    for (int i = 0; i < off && q; i++, q = q->next)
        ;

    return q;
} /*}}}*/

/* Remove the job at the front of the queue, and return it.
 * Returns NULL if the queue is empty. */
queued_job *deq_job(queued_job **q) { /*{{{*/
    queued_job *ret = *q;
    *q = (ret ? ret->next : NULL);

    return ret;
} /*}}}*/

/* Search the queue for a job with a given UUID. */
queued_job *find_job(queued_job **q, const uuid_t uuid) {
    for (queued_job *ret = *q; ret; ret = ret->next) {
        if (uuid == ret->job.job_uuid) {
            return ret;
        }
    }

    return NULL; /* Not found */
}

/* Remove a job from the middle of the queue with a given UUID. */
queued_job *remove_job(queued_job **q, const uuid_t uuid) {/*{{{*/
    queued_job *ret = *q;
    queued_job *prev = NULL;

    for (; ret; ret = ret->next) {
        if (uuid == ret->job.job_uuid) {
            if (prev) prev->next = ret->next;
            else *q = ret->next;
            return ret;
        }
        prev = ret;
    }

    return NULL; /* Not found */
} /*}}}*/

/* Pushes a new job to the back of the queue.
 * Returns the new length of the queue. */
int enq_job(queued_job **q, const job_descriptor job) { /*{{{*/
    queued_job *qjob = malloc(sizeof(queued_job));

    if (!qjob) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    qjob->job = job;
    qjob->t_ended = 0;
    qjob->t_started = 0;
    qjob->manually_released = 0;
    qjob->next = NULL;

    if (!*q) {
        *q = qjob;
        return 1;
    }

    int i;
    for (i = 1; (*q)->next; q = &(*q)->next, i++)
        ;

    (*q)->next = qjob;
    return i;
} /*}}}*/

/* Get length of queue. */
int queue_len(queued_job *q) {/* {{{ */
    int qlen = 0;
    queued_job *qjob;
    if (q) {
        for (qlen = 1, qjob = q; qjob->next; qjob = qjob->next, qlen++)
            ;
    }
    return qlen;
} /* }}} */
