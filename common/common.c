#include "resman.h"

/* This socket address is actually used as an abstract socket address.
 * This means that when it's passed to `bind`, etc., it's prefixed with a NULL
 * byte, and then doesn't have a filesystem representation at all. */
const char *socket_addr = "/resman.socket";
