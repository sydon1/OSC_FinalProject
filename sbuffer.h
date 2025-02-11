//
// Created by sodir on 12/17/24.
//

#ifndef SBUFFER_H
#define SBUFFER_H

#include "config.h"

#define SBUFFER_FAILURE -1
#define SBUFFER_SUCCESS 0
#define SBUFFER_NO_DATA 1

typedef struct sbuffer sbuffer_t;

/**
 * Allocates and initializes a new shared buffer
 * \param buffer a double pointer to the buffer that needs to be initialized
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_init(sbuffer_t **buffer);

/**
 * All allocated resources are freed and cleaned up
 * \param buffer a double pointer to the buffer that needs to be freed
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_free(sbuffer_t **buffer);

/**
 * Removes the first sensor data in 'buffer' (at the 'head') and returns this sensor data as '*data'
 * If 'buffer' is empty, the function doesn't block until new sensor data becomes available but returns SBUFFER_NO_DATA
 * \param buffer a pointer to the buffer that is used
 * \param data a pointer to pre-allocated sensor_data_t space, the data will be copied into this structure. No new memory is allocated for 'data' in this function.
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occurred
 */
int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data);

/**
 * Inserts the sensor data in 'data' at the end of 'buffer' (at the 'tail')
 * \param buffer a pointer to the buffer that is used
 * \param data a pointer to sensor_data_t data, that will be copied into the buffer
 * \return SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occured
*/
int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data);
/**
 * Reads sensor data from a specific processing stage
 * Updates the stage to the next level after reading
 * @param buffer Pointer to the buffer
 * @param data Pointer to store the read sensor data
 * @param stage_id
 * @return SBUFFER_SUCCESS on success, SBUFFER_FAILURE on error, SBUFFER_NO_DATA if buffer is empty
 */
int sbuffer_read(sbuffer_t *buffer, sensor_data_t *data, int stage_id);

/**
 * Check if buffer is fully processed and ready for shutdown
 * \param buffer a pointer to the buffer
 * \return true if buffer is ready for shutdown
 */
bool sbuffer_is_empty(sbuffer_t *buffer);
#endif //SBUFFER_H
