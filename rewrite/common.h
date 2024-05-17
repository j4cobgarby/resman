#ifndef INCLUDE_COMMON_H
#define INCLUDE_COMMON_H

#include <stdint.h>
#include <sys/types.h>

#define SOCKET_NAME "/tmp/resman.socket"
#define BUFFER_SIZE 128

typedef struct {
  pid_t pid;
  uid_t uid;
  char *msg;
} job_msg_t;

int serialise_job(job_msg_t *, char *);
int deserialise_job(job_msg_t *, const char *);

void show_buff(const char *, int);

#endif
