#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>

#define BUFSZ 256

void cleanup(int sig) {
    mq_unlink("/res_request");

    exit(sig < 0 ? EXIT_FAILURE : EXIT_SUCCESS);
}

int main() {
    struct mq_attr attr;
    mqd_t q;

    char buff[BUFSZ + 1];
    ssize_t nread;

    attr.mq_maxmsg = 10;
    attr.mq_msgsize = BUFSZ;

    mq_unlink("/res_request");
    q = mq_open("/res_request", O_RDONLY | O_CREAT | O_EXCL, 0644, &attr);

    if (q == (mqd_t) -1) {
        fprintf(stderr, "Error making queue (%d): %s\n", errno, strerror(errno));
        cleanup(-1);
    }

    signal(SIGINT, cleanup);
    printf("Waiting.\n");

    while ((nread = mq_receive(q, buff, BUFSZ, NULL)) != -1) {
        printf("Got %zd bytes: %s\n", nread, buff);
        sleep(3);
    }

    fprintf(stderr, "Error receiving: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
}
