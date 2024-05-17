#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "../common.h"

int conn_sock;

int mk_and_bind() {
  struct sockaddr_un name;
  int sock;

  sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (sock == -1) {
    perror("connect_to_daemon: socket");
    exit(EXIT_FAILURE);
  }

  memset(&name, 0, sizeof(name));
  name.sun_family = AF_UNIX;
  strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);

  /* Delete sock file in case it exists */
  unlink(SOCKET_NAME);

  if (bind(sock, (const struct sockaddr *)&name, sizeof(name)) == -1) {
    perror("connect_to_daemon: bind");
    exit(EXIT_FAILURE);
  }

  return sock;
}

int main() {
  int dat_sock;
  char buff[BUFFER_SIZE];
  job_msg_t job;

  printf("== Starting resman daemon\n");
  conn_sock = mk_and_bind();

  if (listen(conn_sock, 20) == -1) {
    perror("main: listen");
    exit(EXIT_FAILURE);
  }

  for (;;) {
    dat_sock = accept(conn_sock, NULL, NULL);
    if (dat_sock == -1) {
      perror("main: accept");
      exit(EXIT_FAILURE);
    }

    if (read(dat_sock, buff, sizeof(buff)) == -1) {
      perror("main: read");
      exit(EXIT_FAILURE);
    }

    deserialise_job(&job, buff);

    printf("= Got job:\n"
           "\tpid = %u\n"
           "\tuid = %u\n"
           "\tmsg = %s\n",
           job.pid, job.uid, job.msg);

    kill(job.pid, SIGCONT);
    close(dat_sock);
  }
}
