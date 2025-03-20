#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "resman.h"

const char *socket_addr = "/tmp/resman.sock";

void free_job_descriptor(job_descriptor *job) {
    free(job);
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
