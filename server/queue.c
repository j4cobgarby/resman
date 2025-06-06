// vim: fdm=marker
#include <stdlib.h>
#include "resman.h"
#include "server.h"

/* Take a pointer to the job at the front of the queue, without removing it.
 * An offset can be specified, if a job further in the queue is needed.
 * Returns NULL if the queue is the requested queue position is empty. */
const queued_job *peek_job(queued_job *q, int off) { /*{{{*/
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

/* Remove a job from the middle of the queue with a given UUID. */
queued_job *remove_job(queued_job **q, uuid_t uuid) {/*{{{*/
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
}/*}}}*/

/* Pushes a new job to the back of the queue.
 * Returns the new length of the queue. */
int enq_job(queued_job **q, job_descriptor job) { /*{{{*/
    queued_job *qjob = (queued_job *)malloc(sizeof(queued_job));

    if (!qjob) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    qjob->job = job;
    qjob->t_ended = 0;
    qjob->t_started = 0;

    if (!*q) {
        *q = qjob;
        qjob->next = NULL;
        return 1;
    }

    int i;
    for (i = 1; (*q)->next; q = &(*q)->next, i++)
        ;

    (*q)->next = qjob;
    qjob->next = NULL;
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
}/* }}} */
