#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "../common.h"

static char *cmd;

void cont_handler(int sig) {
  if (sig == SIGCONT) {
    execl("/bin/sh", "sh", "-c", cmd, NULL);
    exit(EXIT_SUCCESS);
  }
}

int connect_to_daemon() {
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

  if (connect(sock, (const struct sockaddr *)&name, sizeof(name)) == -1) {
    perror("connect_to_daemon: connect");
    exit(EXIT_FAILURE);
  }

  return sock;
}

int main(int argc, char **argv) {
  size_t cmd_len = argc - 1;
  job_msg_t job;
  char msg_buff[BUFFER_SIZE];

  for (int i = 1; i < argc; i++)
    cmd_len += strlen(argv[i]);

  cmd = calloc(cmd_len + 1, sizeof(char));

  for (int i = 1; i < argc; i++) {
    strcat(cmd, argv[i]);
    if (i != argc - 1) {
      strcat(cmd, " ");
    }
  }

  printf("Queing command: '%s'\n", cmd);

  // if (daemon(1, 0) < 0) {
  //   fprintf(stderr,
  //           "Failed to daemonize (%d), so you will probably leave the queue
  //           if " "your terminal dies.\n", errno);
  // }

  if (signal(SIGCONT, &cont_handler) == SIG_ERR) {
    fprintf(stderr, "Unexpected error: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* Redirect stdout and stderr to files that can be read after
   * execution */

  // fclose(stdin);
  // freopen("/tmp/resman_stdout", "w", stdout);
  // freopen("/tmp/resman_stderr", "w", stderr);

  job.pid = getpid();
  job.uid = getuid();
  job.msg = "Description of task...";

  unsigned msg_len = serialise_job(&job, msg_buff);

  int sock = connect_to_daemon();
  if (write(sock, msg_buff, msg_len) == -1) {
    perror("main: write");
    exit(EXIT_FAILURE);
  }
  close(sock);

  /* Wait for wakeup, in a loop in case it's woken up spuriously */
  while (1)
    pause();

  exit(EXIT_FAILURE);
}
