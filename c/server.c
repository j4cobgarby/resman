#include "include/resman.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <stdlib.h>

const char *addr = "/tmp/resman.sock";

int main() {
    int sock_fd;
    struct sockaddr_un sockaddr_un = {0};

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
	fprintf(stderr, "Failed to create UNIX socket.\n");
	return EXIT_FAILURE;
    }

    remove(addr);

    sockaddr_un.sun_family = AF_UNIX;
    strcpy(sockaddr_un.sun_path, addr);

    if (bind(sock_fd, (struct sockaddr *)&sockaddr_un, sizeof(struct sockaddr_un)) < 0) {
	fprintf(stderr, "Failed to bind socket.\n");
	return EXIT_FAILURE;
    }
}
