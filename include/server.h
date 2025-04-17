// vim: fdm=marker
#include "resman.h"
#include <pthread.h> /* pthread_mutex_* */
#include <systemd/sd-journal.h>
#include <sys/syslog.h>

#define LISTEN_QUEUE 8
#define POLL_DELAY 2

#define RESMAND_INFO(...) \
    sd_journal_print(LOG_INFO, __VA_ARGS__)
#define RESMAND_ERROR(...) \
    sd_journal_print(LOG_ERR, __VA_ARGS__)

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
queued_job *remove_job(queued_job **q, uuid_t uuid);
int send_queue_info(int soc_client, unsigned int count);
