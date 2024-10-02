#include "resman.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

const char *socket_addr = "/tmp/resman.sock";

void free_job_descriptor(job_descriptor *job) {
    free(job->msg);
}

int ser32(char *b, uint32_t x) {
    for (int j = 3; j >= 0; j--) {
        b[j] = x & 0xff;
        x >>= 8;
    }
    return 4;
}

int ser64(char *b, uint64_t x) {
    for (int j = 7; j >= 0; j--) {
        b[j] = x & 0xff;
        x >>= 8;
    }
    return 8;
}

int deser32(const char *b, uint32_t *x) {
    for (int j = 0; j < 4; j++) {
        *x <<= 8;
        *x |= (uint8_t)b[j];
    }
    return 4;
}

int deser64(const char *b, uint64_t *x) {
    for (int j = 0; j < 8; j++) {
        *x <<= 8;
        *x |= (uint8_t)b[j];
    }
    return 8;
}

/*  Serialises given job into buffer. The size of the buffer is specified with
 *  `len`; if len is not large enough for the buffer, -1 is returned. */
int serialise_job(char *buf, size_t len, job_descriptor *job) {
    unsigned long msg_len = job->msg ? strlen(job->msg) : 0;
    unsigned long req_bytes = JOB_SER_BASELEN + msg_len;
    unsigned long i = 0;

    if (req_bytes > len) {
        /* Buffer not large enough */
        return -1;
    }

    if (req_bytes > JOB_SER_MAXLEN) {
        /* Message was too long */
        return -1;
    }

    buf[i++] = job->req_type;

    i += ser32(&buf[i], job->uid);
    i += ser64(&buf[i], job->t_submitted);
    i += ser64(&buf[i], job->t_started);
    i += ser64(&buf[i], job->t_ended);

    if (job->req_type == JOB_CMD) {
        i += ser32(&buf[i], job->cmd.pid);
    } else if (job->req_type == JOB_TIMESLOT) {
        i += ser32(&buf[i], job->timeslot.secs);
    } else {
        return -1;
    }

    buf[i++] = (uint8_t)msg_len;

    for (unsigned long j = 0; j < msg_len; j++) {
        buf[i++] = job->msg[j];
    }

    assert(i == req_bytes);
    return i;
}

int deserialise_job(const char *buf, size_t len, job_descriptor *job) {
    unsigned long i = 0;
    size_t msg_len;

    job->req_type = (enum job_type)buf[i++];

    i += deser32(&buf[i], &job->uid);
    i += deser64(&buf[i], (uint64_t*)&job->t_submitted);
    i += deser64(&buf[i], (uint64_t*)&job->t_started);
    i += deser64(&buf[i], (uint64_t*)&job->t_ended);

    if (job->req_type == JOB_CMD) {
        i += deser32(&buf[i], (uint32_t*)&job->cmd.pid);
    } else if (job->req_type == JOB_TIMESLOT) {
        i += deser32(&buf[i], (uint32_t*)&job->timeslot.secs);
    } else {
        return -1;
    }

    msg_len = (size_t)buf[i++];

    job->msg = malloc(msg_len + 1);
    if (job->msg == NULL) {
        return -1;
    }

    memcpy(job->msg, &buf[i], msg_len);
    job->msg[msg_len] = 0;
    i += msg_len;

    assert(i == len);

    return i;
}
