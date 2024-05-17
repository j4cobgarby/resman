#include "common.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *ser_uint32(char *buff, uint32_t i) {
  *buff++ = (char)i & 0xff;
  *buff++ = (char)(i >> 8) & 0xff;
  *buff++ = (char)(i >> 16) & 0xff;
  *buff++ = (char)(i >> 24) & 0xff;
  return buff;
}

uint32_t des_uint32(const char **buff) {
  uint32_t ret = 0;

  ret |= (*(*buff)++ & 0xff);
  ret |= (*(*buff)++ & 0xff) << 8;
  ret |= (*(*buff)++ & 0xff) << 16;
  ret |= (*(*buff)++ & 0xff) << 24;

  return ret;
}

char *ser_str(char *buff, const char *str) { return stpcpy(buff, str); }

void des_str(char *out, const char **buff) {
  strcpy(out, *buff);
  *buff += strlen(*buff);
}

unsigned ser_len(job_msg_t *job) { return 16 + strlen(job->msg) + 1; }

int serialise_job(job_msg_t *job, char *buff) {
  unsigned len = ser_len(job);
  char *ptr = buff;
  ptr = ser_uint32(ptr, len);
  ptr = ser_uint32(ptr, job->pid);
  ptr = ser_uint32(ptr, job->uid);
  ptr = ser_uint32(ptr, strlen(job->msg));
  ptr = ser_str(ptr, job->msg);

  assert(len == (unsigned)(ptr - buff) + 1);

  return len;
}

int deserialise_job(job_msg_t *job, const char *buff) {
  unsigned len = des_uint32(&buff);
  job->pid = des_uint32(&buff);
  job->uid = des_uint32(&buff);

  unsigned msg_len = des_uint32(&buff);
  job->msg = malloc(msg_len + 1);
  if (!job->msg)
    return 0;

  des_str(job->msg, &buff);
  return 1;
}

void show_buff(const char *buff, int n) {
  printf("Buff (len %d): ", n);
  for (int i = 0; i < n; i++) {
    printf("[%d]", buff[i] & 0xff);
  }
  printf("\n");
}
