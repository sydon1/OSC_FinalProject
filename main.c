//
// Created by sodir on 12/9/24.
//
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include "config.h"
#include "sbuffer.h"
#include "connmgr.h"
#include "datamgr.h"
#include "sensor_db.h"

int fd[2];
pid_t pid;
FILE *log_file = NULL;
int log_sequence = 0;
// https://stackoverflow.com/questions/14320041/pthread-mutex-initializer-vs-pthread-mutex-init-mutex-param
// while debugging I landed on this form of init. Cleaner in a big file imo.
pthread_mutex_t pipemutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shutdown_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t shutdown_complete;
int active_threads = 0;

void increment_active_threads() {
    pthread_mutex_lock(&shutdown_mutex);
    active_threads++;
    pthread_mutex_unlock(&shutdown_mutex);
}

void decrement_active_threads() {
    pthread_mutex_lock(&shutdown_mutex);
    active_threads--;
    if (active_threads == 0) {
        pthread_cond_signal(&shutdown_complete);
    }
    pthread_mutex_unlock(&shutdown_mutex);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Wrong number of arguments\nUsage: %s <port> <max_connections>\n", argv[0]);
        return -1;
    }
    if (pthread_cond_init(&shutdown_complete, NULL) != 0) {
        printf("Failed to initialize condition variable\n");
        return -1;
    }

    int tcp_port = atoi(argv[1]);
    int max_conn = atoi(argv[2]);

    if (tcp_port < 1024 || max_conn <= 0) {
        printf("Invalid arguments: port must be >= 1024, max connections must be > 0\n");
        return -1;
    }

    if (create_log_process() != 0) {
        printf("Failed to create logging process\n");
        return -1;
    }
    write_to_log_process("Started the gateway");

    sbuffer_t *shared_buffer = NULL;
    if (sbuffer_init(&shared_buffer) != SBUFFER_SUCCESS) {
        write_to_log_process("Failed to initialize shared buffer");
        end_log_process();
        return -1;
    }

    connection_manager_arguments_t *conn_params = malloc(sizeof(connection_manager_arguments_t));
    datamanager_arguments_t *data_params = malloc(sizeof(datamanager_arguments_t));
    storagemanager_arguments_t *storage_params = malloc(sizeof(storagemanager_arguments_t));

    if (!conn_params || !data_params || !storage_params) {
        write_to_log_process("Failed to allocate thread parameters");
        free(conn_params);
        free(data_params);
        free(storage_params);
        sbuffer_free(&shared_buffer);
        end_log_process();
        return -1;
    }

    //shared param setup
    conn_params->port = tcp_port;
    conn_params->max_con = max_conn;
    conn_params->sBuffer = shared_buffer;
    data_params->sBuffer = shared_buffer;
    storage_params->sBuffer = shared_buffer;

    char log_message[300];
    snprintf(log_message, sizeof(log_message), "Initializing with port %d and max connections %d", tcp_port, max_conn);
    write_to_log_process(log_message);

    pthread_t connmgr_thread, datamgr_thread, storagemgr_thread;
    increment_active_threads(); // connection manager
    increment_active_threads(); // data manager
    increment_active_threads(); // storage manager

    if (pthread_create(&connmgr_thread, NULL, connection_manager, conn_params) != 0 ||
        pthread_create(&datamgr_thread, NULL, data_manager, data_params) != 0 ||
        pthread_create(&storagemgr_thread, NULL, storage_manager, storage_params) != 0) {
        write_to_log_process("Failed to create one or more threads");
        free(conn_params);
        free(data_params);
        free(storage_params);
        sbuffer_free(&shared_buffer);
        end_log_process();
        return -1;
    }

    pthread_join(connmgr_thread, NULL);
    write_to_log_process("Connection manager thread completed");
    decrement_active_threads();

    pthread_join(datamgr_thread, NULL);
    write_to_log_process("Data manager thread completed");
    decrement_active_threads();

    pthread_join(storagemgr_thread, NULL);
    write_to_log_process("Storage manager thread completed");
    decrement_active_threads();

    // Wait for all threads to complete cleanly
    pthread_mutex_lock(&shutdown_mutex);
    while (active_threads > 0) {
        pthread_cond_wait(&shutdown_complete, &shutdown_mutex);
    }
    pthread_mutex_unlock(&shutdown_mutex);

    // Final cleanup
    write_to_log_process("Gateway shutting down");
    free(conn_params);
    free(data_params);
    free(storage_params);
    sbuffer_free(&shared_buffer);
    end_log_process();

    pthread_mutex_destroy(&shutdown_mutex);
    pthread_cond_destroy(&shutdown_complete);

    return 0;
}

// Logging process functions
void run_logging_process(void) {
    char buffer;
    char message[LOG_MSG_MAX_LEN] = {0};
    int msg_pos = 0;
    time_t now;

    log_file = fopen(LOG_FILE, "w");
    if (log_file == NULL) {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }

    if (setvbuf(log_file, NULL, _IOLBF, 0) != 0) {
        perror("Failed to set buffer mode");
        fclose(log_file);
        exit(EXIT_FAILURE);
    }

    while (read(fd[0], &buffer, 1) > 0) {
        if (buffer == '\0') {
            time(&now);
            fprintf(log_file, "%d - %.24s - %s\n", log_sequence++, ctime(&now), message);
            msg_pos = 0;
            memset(message, 0, sizeof(message));
        }
        else if (msg_pos < (int)(sizeof(message) - 1)) {
            message[msg_pos++] = buffer;
        }
    }

    fclose(log_file);
    close(fd[0]);
}

int create_log_process(void) {
    if (pipe(fd) == -1) {
        perror("Pipe creation failed");
        return ERR_PIPE;
    }

    pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        close(fd[0]);
        close(fd[1]);
        return ERR_FORK;
    }

    if (pid > 0) {
        close(fd[0]);  // Parent closes read end
    } else {
        close(fd[1]);  // Child closes write end
        run_logging_process();
        exit(EXIT_SUCCESS);
    }
    return SUCCESS;
}

int write_to_log_process(char *msg) {
    if (msg == NULL) return ERR_FILE_IO;

    pthread_mutex_lock(&pipemutex);
    ssize_t bytes = write(fd[1], msg, strlen(msg) + 1);
    pthread_mutex_unlock(&pipemutex);

    return (bytes > 0) ? SUCCESS : ERR_FILE_IO;
}

int end_log_process(void) {
    if (pid > 0) {
        close(fd[1]);
        wait(NULL);
        pthread_mutex_destroy(&pipemutex);
    }
    return SUCCESS;
}