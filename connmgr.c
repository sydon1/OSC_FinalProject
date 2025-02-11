//
// Created by sodir on 12/25/24.
//
#include "connmgr.h"

#include <pthread.h>
#include <stdbool.h>

#include "lib/tcpsock.h"
#include <stdio.h>
#include <stdlib.h>
#include <bits/pthreadtypes.h>

#include "sbuffer.h"

sbuffer_t *sBuffer;

#include "connmgr.h"
#include <string.h>

void *connection_manager(void *args) {
    connmgr_args_t *params = (connmgr_args_t*)args;
    tcpsock_t *server = NULL;
    int active_connections = 0;

    write_to_log_process("Connection manager started");

    // Initialize server socket
    if (tcp_passive_open(&server, params->port) != TCP_NO_ERROR) {
        write_to_log_process("Failed to open server socket");
        return NULL;
    }

    pthread_t *client_threads = malloc(sizeof(pthread_t) * params->max_con);
    client_thread_args_t *thread_args = malloc(sizeof(client_thread_args_t) * params->max_con);

    // Accept connections until max_con is reached
    while (active_connections < params->max_con) {
        thread_args[active_connections].buffer = params->buffer;
        thread_args[active_connections].conn_id = active_connections;

        if (tcp_wait_for_connection(server, &(thread_args[active_connections].client)) != TCP_NO_ERROR) {
            continue;
        }

        // Create thread for new client
        if (pthread_create(&client_threads[active_connections], NULL, handle_client,
                         &thread_args[active_connections]) != 0) {
            write_to_log_process("Failed to create client thread");
            tcp_close(&thread_args[active_connections].client);
            continue;
        }

        active_connections++;
    }

    // Wait for all client threads to finish
    for (int i = 0; i < active_connections; i++) {
        pthread_join(client_threads[i], NULL);
    }

    // Insert end marker in buffer
    sensor_data_t end_marker = {.id = 0};
    sbuffer_insert(params->buffer, &end_marker);

    // Cleanup
    free(client_threads);
    free(thread_args);
    tcp_close(&server);

    write_to_log_process("Connection manager shutting down");
    return NULL;
}

void *handle_client(void *args) {
    client_thread_args_t *client_args = (client_thread_args_t*)args;
    sensor_data_t data;
    int bytes;
    bool first_msg = true;
    char log_message[128];

    while (1) {
        // Read sensor ID
        bytes = sizeof(data.id);
        int result = tcp_receive_timeout(client_args->client, &data.id, &bytes, TIMEOUT);

        if (first_msg && result == TCP_NO_ERROR) {
            snprintf(log_message, sizeof(log_message), "Sensor node %d has opened a new connection", data.id);
            write_to_log_process(log_message);
            first_msg = false;
        }

        if (result != TCP_NO_ERROR) {
            // Handle disconnection/timeout
            if (!first_msg) {  // Only log if we had a valid connection
                snprintf(log_message, sizeof(log_message),
                        "Sensor node %d has %s",
                        data.id,
                        (result == TCP_TIMEOUT_ERROR) ? "timed out" : "closed the connection");
                write_to_log_process(log_message);
            }
            break;
        }

        // Read temperature
        bytes = sizeof(data.value);
        if (tcp_receive(client_args->client, &data.value, &bytes) != TCP_NO_ERROR) break;

        // Read timestamp
        bytes = sizeof(data.ts);
        if (tcp_receive(client_args->client, &data.ts, &bytes) != TCP_NO_ERROR) break;

        // Insert into buffer
        sbuffer_insert(client_args->buffer, &data);
    }

    tcp_close(&client_args->client);
    return NULL;
}