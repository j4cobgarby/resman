// vim: fdm=marker
#include "resman.h"

#define LISTEN_QUEUE 8
#define POLL_DELAY 2

int make_soc_listen(const char *addr);

int handle_client(int soc_client);

void sigint_handler(int sig);

const job_descriptor *peek_job(job_descriptor *q, int off);

job_descriptor *deq_job(job_descriptor **q);

int enq_job(job_descriptor **q, job_descriptor *job);
