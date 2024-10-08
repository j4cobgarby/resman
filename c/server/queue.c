// vim: fdm=marker
#include "server.h"

/* Take a pointer to the job at the front of the queue, without removing it.
 * An offset can be specified, if a job further in the queue is needed.
 * Returns NULL if the queue is the requested queue position is empty. */
const job_descriptor *peek_job(job_descriptor *q, int off) { /*{{{*/
    for (int i = 0; i < off && q; i++, q = q->next)
        ;

    return q;
} /*}}}*/

/* Remove the job at the front of the queue, and return it.
 * Returns NULL if the queue is empty. */
job_descriptor *deq_job(job_descriptor **q) { /*{{{*/
    job_descriptor *ret = *q;

    *q = (ret ? ret->next : NULL);

    return ret;
} /*}}}*/

/* Pushes a new job to the back of the queue.
 * Returns the new length of the queue. */
int enq_job(job_descriptor **q, job_descriptor *job) { /*{{{*/
    int i;

    if (!*q) {
        *q = job;
        job->next = NULL;
        return 1;
    }

    for (i = 1; (*q)->next; q = &(*q)->next, i++)
        ;

    (*q)->next = job;
    job->next = NULL;
    return i;
} /*}}}*/
