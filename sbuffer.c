//
// Created by sodir on 12/9/24.
//
#include "sbuffer.h"
#include "config.h"

/**
 * basic node for the buffer, these nodes are linked together to create the buffer
 */
typedef struct sbuffer_node {
    struct sbuffer_node *next;
    sensor_data_t data;
    int processed_stages;
} sbuffer_node_t;

/**
 * a structure to keep track of the buffer
    We will use mutex locks with condition variables.
    Each time we lock before we get to the critical section
 */
struct sbuffer {
    sbuffer_node_t *head;       /**< a pointer to the first node in the buffer */
    sbuffer_node_t *tail;       /**< a pointer to the last node in the buffer */
};

pthread_mutex_t bufferMutex;
pthread_cond_t dataAvailable;
pthread_cond_t stageComplete;

int sbuffer_init(sbuffer_t **buffer) {
    *buffer = malloc(sizeof(sbuffer_t));
    if (*buffer == NULL) return SBUFFER_FAILURE;
    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;

    pthread_mutex_init(&bufferMutex, NULL);
    pthread_cond_init(&dataAvailable, NULL);
    pthread_cond_init(&stageComplete, NULL);

    return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **buffer) {
    pthread_mutex_lock(&bufferMutex);
    if ((buffer == NULL) || (*buffer == NULL)) {
        pthread_mutex_unlock(&bufferMutex);
        return SBUFFER_FAILURE;
    }

    while ((*buffer)->head) {
        sbuffer_node_t *dummy = (*buffer)->head;
        (*buffer)->head = (*buffer)->head->next;
        free(dummy);
    }
    pthread_mutex_unlock(&bufferMutex);

    pthread_cond_destroy(&dataAvailable);
    pthread_mutex_destroy(&bufferMutex);
    pthread_cond_destroy(&stageComplete);

    free(*buffer);
    *buffer = NULL;
    return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data) {
    if (buffer == NULL) return SBUFFER_FAILURE;
    pthread_mutex_lock(&bufferMutex);

    sbuffer_node_t *node = malloc(sizeof(sbuffer_node_t));
    if (node == NULL) return SBUFFER_FAILURE;

    node->data = *data;
    node->next = NULL;
    node->processed_stages = 0; //set stages, better than enums


    if (buffer->tail == NULL) {
        buffer->head = buffer->tail = node;
    } else {
        buffer->tail->next = node;
        buffer->tail = node;
    }

    pthread_cond_broadcast(&dataAvailable);
    pthread_mutex_unlock(&bufferMutex);

    return SBUFFER_SUCCESS;
}

int sbuffer_read(sbuffer_t *buffer, sensor_data_t *data, int stage_id) {
    pthread_mutex_lock(&bufferMutex);

    while (1) {
        if (buffer->head == NULL) {
            pthread_cond_wait(&dataAvailable, &bufferMutex);
            continue;
        }

        // Check for end marker
        if (buffer->head->data.id == 0) {
            *data = buffer->head->data;
            pthread_mutex_unlock(&bufferMutex);
            return SBUFFER_NO_DATA;
        }

        //check if node already processed
        int stage_bit = (1 << (stage_id - 1));
        if (buffer->head->processed_stages & stage_bit) {
            pthread_cond_wait(&dataAvailable, &bufferMutex);
            continue;
        }

        *data = buffer->head->data;
        buffer->head->processed_stages |= stage_bit;

        pthread_cond_signal(&stageComplete);
        pthread_mutex_unlock(&bufferMutex);
        return SBUFFER_SUCCESS;
    }
}

int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data) {
    pthread_mutex_lock(&bufferMutex);

    while (buffer->head == NULL || buffer->head->processed_stages != 3) {
        pthread_cond_wait(&stageComplete, &bufferMutex);
    }

    *data = buffer->head->data;
    sbuffer_node_t *temp = buffer->head;
    buffer->head = buffer->head->next;

    if (buffer->head == NULL) {
        buffer->tail = NULL;
    }

    free(temp);
    pthread_mutex_unlock(&bufferMutex);

    return buffer->head == NULL? SBUFFER_NO_DATA : SBUFFER_SUCCESS;
}

bool sbuffer_is_empty(sbuffer_t *buffer) {
    if (!buffer) return true;

    pthread_mutex_lock(&bufferMutex);
    bool is_empty = (buffer->head == NULL);
    pthread_mutex_unlock(&bufferMutex);

    return is_empty;
}