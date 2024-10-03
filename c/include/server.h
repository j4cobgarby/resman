// vim: fdm=marker
#include "resman.h"

#define LISTEN_QUEUE 8

int make_soc_listen(const char *addr);

int handle_client(int soc_client);
