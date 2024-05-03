#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <mqueue.h>
#include <signal.h>

void cont_handler(int sig) {
    if (sig != SIGCONT) return;
}

int main() {
    mqd_t q = mq_open("/res_request", O_WRONLY);

    if (q == (mqd_t) -1) {
        fprintf(stderr, "Error making queue: %s\n", strerror(errno));
    }

    const char *str = "Hello, world! This is my message.";
    mq_send(q, str, strlen(str), 0);
}
