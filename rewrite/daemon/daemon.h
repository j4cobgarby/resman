#ifndef INCLUDE_DAEMON_H
#define INCLUDE_DAEMON_H

#include <stddef.h> /* ssize_t */
#include <mqueue.h> /* mqd_t, mq_* */
#include "../common.h"

mqd_t make_queue(const char *name);

ssize_t queue_fetch(char *buffer);

#endif
