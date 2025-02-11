//
// Created by sodir on 12/25/24.
//

#ifndef SENSOR_DB_H
#define SENSOR_DB_H
#include <stdio.h>
#include "config.h"
#include <stdbool.h>

typedef struct {
 sbuffer_t *buffer;
} storage_args_t;

void *storage_manager(void *args);
FILE *open_db(const char *filename);
int write_sensor_data(FILE *fp, sensor_data_t *data);
void close_db(FILE *fp);

#endif //SENSOR_DB_H
