//
// Created by sodir on 12/25/24.
//

#ifndef DATAMGR_H
#define DATAMGR_H
#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "lib/dplist.h"

/*
 * Use ERROR_HANDLER() for handling memory allocation problems, invalid sensor IDs, non-existing files, etc.
 */
#define ERROR_HANDLER(condition, ...)    do {                       \
                      if (condition) {                              \
                        printf("\nError: in %s - function %s at line %d: %s\n", __FILE__, __func__, __LINE__, __VA_ARGS__); \
                        exit(EXIT_FAILURE);                         \
                      }                                             \
                    } while(0)

void *element_copy(void *element);
void element_free(void **element);
int element_compare(void *x, void *y);

typedef struct {
 sbuffer_t *buffer;
} datamgr_args_t;

typedef struct {
 sensor_id_t sensor_id;
 uint16_t room_id;
 double running_avg[RUN_AVG_LENGTH];
 int current_idx;
 time_t last_modified;
} sensor_data_elem_t;

void *data_manager(void *args);
int parse_sensor_map(dplist_t *list);
void process_sensor_data(dplist_t *list, sensor_data_t *data);
void check_sensor_limits(sensor_data_elem_t *sensor);

/**
 * This method should be called to clean up the datamgr, and to free all used memory.
 * After this, any call to datamgr_get_room_id, datamgr_get_avg, datamgr_get_last_modified or datamgr_get_total_sensors will not return a valid result
 */
void datamgr_free();

/**
 * Gets the room ID for a certain sensor ID
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the corresponding room id
 */
uint16_t datamgr_get_room_id(sensor_id_t sensor_id);

/**
 * Gets the running AVG of a certain senor ID (if less then RUN_AVG_LENGTH measurements are recorded the avg is 0)
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the running AVG of the given sensor
 */
sensor_value_t datamgr_get_avg(sensor_id_t sensor_id);

/**
 * Returns the time of the last reading for a certain sensor ID
 * Use ERROR_HANDLER() if sensor_id is invalid
 * \param sensor_id the sensor id to look for
 * \return the last modified timestamp for the given sensor
 */
time_t datamgr_get_last_modified(sensor_id_t sensor_id);

/**
 *  Return the total amount of unique sensor ID's recorded by the datamgr
 *  \return the total amount of sensors
 */
int datamgr_get_total_sensors();

#endif //DATAMGR_H
