//
// Created by sodir on 12/17/24.
//

#ifndef SENSOR_DB_H
#define SENSOR_DB_H

#include <stdio.h>
#include "config.h"
#include <stdbool.h>

/**
Main storage management thread function

Responsible for reading sensor data from a shared buffer and writing it to data.csv
@param args Thread arguments containing the shared sensor data buffer
@return NULL on completion or error
*/
void *storage_manager(void *args);

/**
Creates or opens an existing file in append mode.

@param filename Name of the database file to open
@return FILE* to the opened file, or NULL on error
*/
FILE *open_db(char *filename);

/**
Write sensor ID, value, and timestamp to the file in csv file

@param fp File pointer to write to
@param data Pointer to the sensor data to write
@return 0 on success, -1 on error
*/
int write_sensor_data(FILE *fp, sensor_data_t *data);

/**
Close the db file
@param f File pointer to close
*/
void close_db(FILE *f);
#endif //SENSOR_DB_H
