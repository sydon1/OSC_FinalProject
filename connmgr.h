#ifndef _CONNMGR_H_
#define _CONNMGR_H_

#include "config.h"
#include "lib/tcpsock.h"
#include "sbuffer.h"
#include <pthread.h>

typedef struct {
 int port;
 int max_con;
 sbuffer_t* buffer;
} connmgr_args_t;

typedef struct {
 tcpsock_t *client;
 sbuffer_t *buffer;
 int conn_id;
} client_thread_args_t;

void *connection_manager(void *args);
void *handle_client(void *args);

#endif