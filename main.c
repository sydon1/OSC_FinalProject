//
// Created by sodir on 12/25/24.
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
pthread_mutex_t pipemutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
    // Argument validation
    if (argc != 3) {
        printf("Wrong number of arguments\nUsage: %s <port> <max_connections>\n", argv[0]);
        return -1;
    }

    int tcp_port = atoi(argv[1]);
    int max_conn = atoi(argv[2]);

    if (tcp_port < 1024 || max_conn <= 0) {
        printf("Invalid arguments: port must be >= 1024, max connections must be > 0\n");
        return -1;
    }

    // Initialize logging
    if (create_log_process() != 0) {
        printf("Failed to create logging process\n");
        return -1;
    }
    write_to_log_process("Started the gateway");

    // Initialize buffer
    sbuffer_t *shared_buffer = NULL;
    if (sbuffer_init(&shared_buffer) != SBUFFER_SUCCESS) {
        write_to_log_process("Failed to initialize shared buffer");
        end_log_process();
        return -1;
    }

    // Parameter initialization
    connmgr_args_t *conn_params = malloc(sizeof(connmgr_args_t));
    datamgr_args_t *data_params = malloc(sizeof(datamgr_args_t));
    storage_args_t *storage_params = malloc(sizeof(storage_args_t));

    if (!conn_params || !data_params || !storage_params) {
        write_to_log_process("Failed to allocate thread parameters");
        free(conn_params);
        free(data_params);
        free(storage_params);
        sbuffer_free(&shared_buffer);
        end_log_process();
        return -1;
    }

    // Initialize parameters
    conn_params->port = tcp_port;
    conn_params->max_con = max_conn;
    conn_params->buffer = shared_buffer;
    data_params->buffer = shared_buffer;
    storage_params->buffer = shared_buffer;

    // Debug log
    char debug_msg[128];
    snprintf(debug_msg, sizeof(debug_msg),
             "Initializing with port %d and max connections %d",
             tcp_port, max_conn);
    write_to_log_process(debug_msg);

    // Thread creation
    pthread_t connmgr_thread, datamgr_thread, storagemgr_thread;
    int thread_create_error = 0;

    // Create threads with error checking
    thread_create_error |= pthread_create(&connmgr_thread, NULL, connection_manager, conn_params);
    thread_create_error |= pthread_create(&datamgr_thread, NULL, data_manager, data_params);
    thread_create_error |= pthread_create(&storagemgr_thread, NULL, storage_manager, storage_params);

    if (thread_create_error != 0) {
        write_to_log_process("Failed to create one or more threads");
        free(conn_params);
        free(data_params);
        free(storage_params);
        sbuffer_free(&shared_buffer);
        end_log_process();
        return -1;
    }

    // Wait for threads in the correct order
    pthread_join(connmgr_thread, NULL);
    write_to_log_process("Connection manager thread completed");

    pthread_join(datamgr_thread, NULL);
    write_to_log_process("Data manager thread completed");

    pthread_join(storagemgr_thread, NULL);
    write_to_log_process("Storage manager thread completed");

    // Final cleanup
    write_to_log_process("Gateway shutting down");
    free(conn_params);
    free(data_params);
    free(storage_params);
    sbuffer_free(&shared_buffer);
    end_log_process();

    return 0;
}

void run_logging_process(void) {
    char buffer;
    char message[LOG_MSG_MAX_LEN] = {0};
    int msg_pos = 0;
    time_t now;

    log_file = fopen(LOG_FILE, "w");  // Changed to "w" to create new log each time
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

    if (errno != 0) {
        perror("Error reading from pipe");
    }

    fclose(log_file);
    close(fd[0]);
    exit(EXIT_SUCCESS);
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
        close(fd[0]);
    }
    else {
        close(fd[1]);
        run_logging_process();
    }
    return SUCCESS;
}

int write_to_log_process(char *msg) {
    if (msg == NULL) {
        return ERR_FILE_IO;
    }
    int result = ERR_FILE_IO;

    if (pthread_mutex_lock(&pipemutex) != 0) {
        perror("Mutex lock failed");
        return ERR_FILE_IO;
    }

    ssize_t bytes = write(fd[1], msg, strlen(msg) + 1);
    if (bytes > 0) {
        result = SUCCESS;
    } else {
        perror("Write to pipe failed");
    }

    if (pthread_mutex_unlock(&pipemutex) != 0) {
        perror("Mutex unlock failed");
        return ERR_FILE_IO;
    }
    return result;
}

int end_log_process(void) {
    if (pid > 0) {
        if (close(fd[1]) == -1) {
            perror("Close pipe failed");
            return ERR_PIPE;
        }
        wait(NULL);

        if (pthread_mutex_destroy(&pipemutex) != 0) {
            perror("Mutex destruction failed");
            return ERR_MEMORY;
        }

        return SUCCESS;
    }
    return SUCCESS;
}
