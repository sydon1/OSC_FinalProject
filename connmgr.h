//
// Created by sodir on 12/17/24.
//

#ifndef CONNMGR_H
#define CONNMGR_H

#include "config.h"
#include "lib/tcpsock.h"
#include "sbuffer.h"
#include <pthread.h>

/**
 * Main thread function for the connection manager
 * - Initializes the TCP server socket
 * - Listens for incoming sensor connections
 * - Creates worker threads for each connected sensor
 * - Manages the lifecycle of sensor connections
 * @param args Pointer to connection manager parameters (connection_manager_arguments_t)
 * @return NULL on completion, all error handling done via logging
 */
void *connection_manager(void *args);

/**
 * Worker thread function for handling individual sensor connections
 * - Reads sensor data from the TCP connection
 * - Processes incoming measurements
 * - Writes data to the shared buffer
 * - Handles connection timeouts and errors
 * @param args Pointer to connection parameters (client_thread_arguments_t)
 * @return NULL on completion
 */
void *connect_connmmgr(void *args);
#endif //CONNMGR_H
