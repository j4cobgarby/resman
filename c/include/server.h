// vim: fdm=marker
#include "resman.h"

#define LISTEN_QUEUE 8
#define POLL_DELAY 2

/* Thin wrapper around job descriptor for server-only fields */
typedef struct queued_job {
    job_descriptor job;
    struct queued_job *next;
    time_t t_started;
    time_t t_ended;
} queued_job;

void free_queued_job(queued_job *);

extern queued_job *running_job;
extern queued_job *q;
extern pthread_mutex_t mut_rj;
extern pthread_mutex_t mut_q;

void *dispatcher(void *args);
int make_soc_listen(const char *addr);
int handle_client(int soc_client);
void sigint_handler(int sig);
const queued_job *peek_job(queued_job *q, int off);
queued_job *deq_job(queued_job **q);
int enq_job(queued_job **q, job_descriptor job);
