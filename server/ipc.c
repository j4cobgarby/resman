// vim: fdm=marker
#include <sys/socket.h>
#include <sys/stat.h> /* chmod */
#include <sys/un.h>
#include <unistd.h>

#include "resman.h"
#include "server.h"

/* Create and return a new UNIX domain socket which listens on a given address.
 * This address is a filesystem path, since this type of socket uses a "file"
 * to communicate over. */
int make_soc_listen(const char *addr) { /*{{{*/
    int soc_listen;
    unsigned int sa_len;
    struct sockaddr_un sa_local = {0};

    soc_listen = socket(AF_UNIX, SOCK_STREAM, 0);
    if (soc_listen < 0) {
        perror("socket");
        return -1;
    }

    memset(&sa_local, 0, sizeof(sa_local));
    sa_local.sun_family = AF_UNIX;
    sa_local.sun_path[0] = '\0';
    strncpy(sa_local.sun_path + 1, addr, strlen(addr));

    sa_len = offsetof(struct sockaddr_un, sun_path) + 1 + strlen(addr);

    if (bind(soc_listen, (struct sockaddr *)&sa_local, sa_len) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(soc_listen, LISTEN_QUEUE) < 0) {
        perror("listen");
        return -1;
    }

    return soc_listen;
} /*}}}*/

/* Get the next job UUID. This will wrap around, but we assume that there won't
 * be so many jobs in the queue that there are ID collisions. */
static uuid_t next_uuid(void) { /*{{{*/
    static uuid_t next = 0;
    if (next == UUID_MAX) next = 0;
    return ++next;
} /*}}}*/

static int send_status(int soc, status_response *resp) {/*{{{*/
    if (!resp) return -1;
    return send(soc, resp, sizeof(status_response), 0);
}/*}}}*/

/* Handle a new client connection. This will wait for the client to send a
 * request, at which point -- based on the type of request -- it will perform
 * the necessary action.
 * Returns -1 on failure, or 0 on success. */
int handle_client(int soc_client) { /*{{{*/
    ipc_request req;
    status_response resp;

    int bytes_read;
    if ((bytes_read = recv(soc_client, &req, sizeof(req), 0)) == -1) {
        perror("recv");
    }

    if (bytes_read != sizeof(req)) {
        RESMAND_ERROR("Problem with client request: got %d bytes but expected %lu\n",
               bytes_read, sizeof(req));
        close(soc_client);
        return -1;
    }

    if (req.req_type == IPCREQ_JOB) {
        job_descriptor job = req.job;

        if (job.job_type != JOB_CMD && job.job_type != JOB_TIMESLOT) {
            RESMAND_ERROR("Invalid job type in request\n");
            goto _close;
        }

        job.job_uuid = next_uuid();

        switch (job.job_type) {
            case JOB_CMD:
                RESMAND_INFO(
                    "Received command job %d by user %d (pid: %d, msg: '%s')\n",
                    job.job_uuid, job.uid, job.cmd.pid, job.msg
                );
                pthread_mutex_lock(&mut_q);
                enq_job(&q, job);
                pthread_mutex_unlock(&mut_q);
                resp.status = STATUS_OK;
                break;
            case JOB_TIMESLOT:
                RESMAND_INFO(
                    "Received timeslot job %d by user %d (time: %ds, msg: '%s')\n",
                    job.job_uuid, job.uid, job.timeslot.secs, job.msg
                );

                if (running_job || peek_job(q, 0)) {
                    RESMAND_INFO(
                        "Rejecting timeslot job %d: server is already reserved\n",
                        job.job_uuid
                    );
                    resp.status = STATUS_FAIL;
                    break;
                }

                pthread_mutex_lock(&mut_q);
                enq_job(&q, job);
                pthread_mutex_unlock(&mut_q);
                resp.status = STATUS_OK;
                break;
        }

        send_status(soc_client, &resp);
        disp_status();
    } else if (req.req_type == IPCREQ_VIEW_QUEUE) {
        info_request info = req.info;
        send_queue_info(soc_client, info.n_view);
    } else if (req.req_type == IPCREQ_DEQUEUE) {
        dequeue_request deq = req.deq;
        RESMAND_INFO("Received dequeue request for job %d\n", deq.job_uuid);

        pthread_mutex_lock(&mut_rj);
        pthread_mutex_lock(&mut_q);

        if (running_job && running_job->job.job_uuid == deq.job_uuid) {
            RESMAND_ERROR("Refusing to dequeue currently running job\n")
            resp.status = STATUS_DEQ_FAIL_JOB_CURRENTLY_RUNNING;
        } else {
            queued_job *deq_job = remove_job(&q, deq.job_uuid);
            if (!deq_job) {
                RESMAND_ERROR("Failed to dequeue job %d: not found in queue"
                              "\n", deq.job_uuid);
                resp.status = STATUS_DEQ_FAIL_NO_SUCH_JOB;
            } else {
                RESMAND_INFO("Dequeueueueueing job %d\n", deq_job->job.job_uuid);
                free_queued_job(deq_job);
                resp.status = STATUS_OK;
            }
        }

        pthread_mutex_unlock(&mut_q);
        pthread_mutex_unlock(&mut_rj);

        send_status(soc_client, &resp);

        disp_status();
    } else if (req.req_type == IPCREQ_RELEASE) {
        release_request rel = req.rel;

        if (!running_job) {
            RESMAND_INFO("Received manual release request by user %d, but "
                         "server is not reserved. Nothing to do.\n",
                         rel.uid)
            resp.status = STATUS_OK;
        } else {
            RESMAND_INFO("Received manual release request by user %d (force: "
                         "%d), cancelling current running job %d\n",
                         rel.uid, rel.force, running_job->job.job_uuid);
            pthread_mutex_lock(&mut_rj);
            running_job->manually_released = 1;
            pthread_mutex_unlock(&mut_rj);
            resp.status = STATUS_OK;
        }
        send_status(soc_client, &resp);
    } else {
        RESMAND_ERROR("Invalid request type: %d\n", req.req_type);
        close(soc_client);
        return -1;
    }
_close:
    close(soc_client);

    return 0;
} /*}}}*/
