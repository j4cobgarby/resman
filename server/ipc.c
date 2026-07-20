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

static ssize_t send_status(const int soc, status_response *resp) { /*{{{*/
    if (!resp) return -1;
    return send(soc, resp, sizeof(status_response), 0);
} /*}}}*/

/* Handle a new client connection. This will wait for the client to send a
 * request, at which point -- based on the type of request -- it will perform
 * the necessary action.
 * Returns -1 on failure, or 0 on success. */
int handle_client(const int soc_client) { /*{{{*/
    ipc_request req;
    status_response resp;

    ssize_t bytes_read;
    if ((bytes_read = recv(soc_client, &req, sizeof(req), 0)) == -1) {
        perror("recv");
    }

    if (bytes_read != sizeof(req)) {
        RESMAND_ERROR(
            "Problem with client request: got %ld bytes but expected %lu\n",
            bytes_read, sizeof(req));
        close(soc_client);
        return -1;
    }

    switch (req.req_type) {
        case IPCREQ_JOB:
            job_descriptor job = req.job;

            if (job.job_type != JOB_CMD && job.job_type != JOB_TIMESLOT) {
                RESMAND_ERROR("Invalid job type in request\n");
                goto _close;
            }

            job.job_uuid = next_uuid();

            switch (job.job_type) {
                case JOB_CMD:
                    RESMAND_INFO(
                        "Received command job %d by user %d (pid: %d, msg: "
                        "'%s')\n",
                        job.job_uuid, job.uid, job.cmd.pid, job.msg);
                    pthread_mutex_lock(&mut_q);
                    enq_job(&q, job);
                    pthread_mutex_unlock(&mut_q);
                    resp.status = STATUS_OK;
                    break;
                case JOB_TIMESLOT:
                    RESMAND_INFO(
                        "Received timeslot job %d by user %d (time: %ds, msg: "
                        "'%s')\n",
                        job.job_uuid, job.uid, job.timeslot.secs, job.msg);

                    if (running_job || peek_job(q, 0)) {
                        RESMAND_INFO(
                            "Rejecting timeslot job %d: server is already "
                            "reserved\n",
                            job.job_uuid);
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
            break;
        case IPCREQ_VIEW_QUEUE:
            const info_request info = req.info;
            send_queue_info(soc_client, info.n_view);
            break;
        case IPCREQ_DEQUEUE:
            const dequeue_request deq = req.deq;

            pthread_mutex_lock(&mut_q);
            pthread_mutex_lock(&mut_rj);

            if (running_job && running_job->job.job_uuid == deq.job_uuid) {
                RESMAND_ERROR(
                    "Received dequeue request by user %d for job %d, "
                    "but refusing to dequeue currently running job\n",
                    deq.uid, deq.job_uuid)
                resp.status = STATUS_DEQ_FAIL_JOB_CURRENTLY_RUNNING;
            } else {
                queued_job *target_job = find_job(&q, deq.job_uuid);
                if (!target_job) {
                    RESMAND_ERROR(
                        "Received dequeue request by user %d for job "
                        "%d, but no such job in queue\n",
                        deq.uid, deq.job_uuid);
                    resp.status = STATUS_DEQ_FAIL_NO_SUCH_JOB;
                } else if (target_job->job.uid != deq.uid && !deq.force) {
                    RESMAND_ERROR(
                        "Received dequeue request by user %d for job "
                        "%d owned by %d (force: %d), refusing to "
                        "dequeue\n",
                        deq.uid, deq.job_uuid, target_job->job.uid, deq.force);
                    resp.status = STATUS_DEQ_FAIL_NOT_YOUR_JOB;
                } else {
                    RESMAND_INFO(
                        "Received dequeue request by user %d for job "
                        "%d owned by %d (force: %d), dequeueing job\n",
                        deq.uid, deq.job_uuid, target_job->job.uid, deq.force);
                    // This searches the queue again (cf. find_job above), but
                    // it's fine since we hold mut_q throughout
                    queued_job *deq_job = remove_job(&q, deq.job_uuid);
                    free_queued_job(deq_job);
                    resp.status = STATUS_OK;
                }
            }

            pthread_mutex_unlock(&mut_rj);
            pthread_mutex_unlock(&mut_q);

            send_status(soc_client, &resp);
            break;
        case IPCREQ_RELEASE:
            const release_request rel = req.rel;

            pthread_mutex_lock(&mut_rj);

            if (!running_job) {
                RESMAND_INFO(
                    "Received manual release request by user %d, but "
                    "server is not reserved. Nothing to do.\n",
                    rel.uid)
                resp.status = STATUS_REL_OK_SERVER_IDLE;
            } else {
                if (running_job->job.uid == rel.uid) {
                    // Requesting user owns the job, ok
                    RESMAND_INFO(
                        "Received manual release request by user %d "
                        "for their currently running job %d, "
                        "releasing lock\n",
                        rel.uid, running_job->job.job_uuid);
                    running_job->manually_released = 1;
                    resp.status = STATUS_OK;
                } else {
                    // Requesting user does not own the job, require force
                    if (rel.force) {
                        RESMAND_INFO(
                            "Received manual release request by user "
                            "%d for current running job %d owned by "
                            "%d (force: %d), releasing lock\n",
                            rel.uid, running_job->job.uid,
                            running_job->job.job_uuid, rel.force);
                        running_job->manually_released = 1;
                        resp.status = STATUS_OK;
                    } else {
                        RESMAND_ERROR(
                            "Received manual release request by user "
                            "%d for current running job %d owned by "
                            "%d (force: %d), refusing to release "
                            "lock\n",
                            rel.uid, running_job->job.job_uuid,
                            running_job->job.uid, rel.force);
                        resp.status = STATUS_REL_FAIL_NOT_YOUR_JOB;
                    }
                }
            }

            pthread_mutex_unlock(&mut_rj);

            send_status(soc_client, &resp);
            break;
        default:
            RESMAND_ERROR("Invalid request type: %d\n", req.req_type);
            close(soc_client);
            return -1;
    }

_close:
    close(soc_client);

    return 0;
} /*}}}*/
