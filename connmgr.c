//
// Created by sodir on 12/9/24.
//
#include "connmgr.h"
#include <pthread.h>
#include <stdbool.h>
#include "lib/tcpsock.h"
#include <stdio.h>
#include <stdlib.h>
#include "sbuffer.h"
#include <string.h>

sbuffer_t *sBuffer;

void *connection_manager(void *args) {
    connection_manager_arguments_t *params = (connection_manager_arguments_t*)args; //use of explicit casting is safer
    tcpsock_t *server = NULL;
    int active_connections = 0;

    write_to_log_process("Connection manager started");

    if (tcp_passive_open(&server, params->port) != TCP_NO_ERROR) {
        write_to_log_process("Failed to open server socket");
        return NULL;
    }

    pthread_t *client_threads = malloc(sizeof(pthread_t) * params->max_con); //correcte aantal opstarten
    client_thread_arguments_t *thread_args = malloc(sizeof(client_thread_arguments_t) * params->max_con);

    while (active_connections < params->max_con) {
        thread_args[active_connections].sBuffer = params->sBuffer;
        thread_args[active_connections].conn_id = active_connections;

        //if we have no errors
        if (tcp_wait_for_connection(server, &(thread_args[active_connections].client)) != TCP_NO_ERROR) {
            continue;
        }

        // Create thread for new client
        if (pthread_create(&client_threads[active_connections], NULL, connect_connmmgr, &thread_args[active_connections]) != 0) {
            write_to_log_process("Failed to create client thread");
            tcp_close(&thread_args[active_connections].client);
            continue;
        }

        active_connections++;
    }

    //finishing all threads
    for (int i = 0; i < active_connections; i++) {
        pthread_join(client_threads[i], NULL);
    }

    //inserting end marker in shared buffer
    sensor_data_t end_marker = {.id = 0};
    sbuffer_insert(params->sBuffer, &end_marker);

    free(client_threads);
    free(thread_args);
    tcp_close(&server);
    write_to_log_process("Connection manager shutting down");
    return NULL;
}

void *connect_connmmgr(void *args) {
    client_thread_arguments_t *client_arguments = (client_thread_arguments_t*)args;
    sensor_data_t data;
    int bytes;
    bool first_msg = true;

    while (1) {
        char log_message[300];
        // read sensor id, check if we are first, also check tcp error
        bytes = sizeof(data.id);
        int result = tcp_receive_with_timeout(client_arguments->client, &data.id, &bytes, TIMEOUT);

        if (first_msg && result == TCP_NO_ERROR) {
          snprintf(log_message, sizeof(log_message), "Sensor node %d has opened a new connection", data.id);
          write_to_log_process(log_message);
          first_msg = false;
        }

        if (result != TCP_NO_ERROR) {
          if (!first_msg) {  // Only log if we had a valid sensor
            if (result == TCP_TIMEOUT_ERROR) {
                snprintf(log_message, sizeof(log_message), "Sensor node %d has timed out", data.id);
            } else if (result == TCP_CONNECTION_CLOSED) {
                snprintf(log_message, sizeof(log_message), "Sensor node %d has closed the connection", data.id);
            } else if (result == TCP_SOCKET_ERROR) {
                snprintf(log_message, sizeof(log_message), "Sensor node %d encountered a socket error", data.id);
            } else {
                snprintf(log_message, sizeof(log_message), "Sensor node %d encountered an error", data.id);
            }
            write_to_log_process(log_message);
          }
         break;
        }
        //reading all other paramaters
        bytes = sizeof(data.value);
        if (tcp_receive_with_timeout(client_arguments->client, &data.value, &bytes, TIMEOUT) != TCP_NO_ERROR)
            break;

        bytes = sizeof(data.ts);
        if (tcp_receive_with_timeout(client_arguments->client, &data.ts, &bytes, TIMEOUT) != TCP_NO_ERROR)
            break;

        //now we insert it in the buffer,
        sbuffer_insert(client_arguments->sBuffer, &data);
    }
    tcp_close(&client_arguments->client);
    return NULL;
}