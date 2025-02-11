//
// Created by sodir on 12/9/24.
//
#include "sensor_db.h"
#include <stdio.h>
#include "config.h"
#include "sbuffer.h"
#include <string.h>

void *storage_manager(void *args) {
    storagemanager_arguments_t *params = (storagemanager_arguments_t*)args; //explicit casting
    sensor_data_t data;

    FILE *fp = open_db("data.csv");
    if (!fp) {
        write_to_log_process("Failed to open data file");
        return NULL;
    }

    write_to_log_process("A new data.csv file has been created.");

    // process data from buffer
    while (1) {
        int result = sbuffer_read(params->sBuffer, &data, 2);  // stage 2 = storage manager

        if (result == SBUFFER_NO_DATA) {
            break;  // End marker received
        }

        if (result == SBUFFER_SUCCESS) {
            if (write_sensor_data(fp, &data) == 0) {
                char log[300];
                snprintf(log, sizeof(log), "Data insertion from sensor %d succeeded", data.id);
                write_to_log_process(log);
            } else {
                write_to_log_process("Failed to write sensor data");
            }
        }

        //remove from buffer after reading/writing
        sbuffer_remove(params->sBuffer, &data);
    }
    close_db(fp);
    write_to_log_process("Storage manager shutting down");
    return NULL;
}

FILE *open_db(char *filename) {
  	if(filename == NULL) {
    	write_to_log_process("db filename is invalid");
    	return NULL;
  	}
    FILE *fp = fopen(filename, "a");
    if (!fp) {
        write_to_log_process("Could not open data file");
        return NULL;
    }
    return fp;
}

int write_sensor_data(FILE *f, sensor_data_t *data) {
    if (!f|| !data) {
      return -1;
    }

    int result = fprintf(f, "%d,%.2f,%ld\n", data->id, data->value, data->ts);

    if (result < 0) {
        return -1;
    }

    fflush(f);
    return 0;
}

void close_db(FILE *f) {
    if (f) {
        write_to_log_process("The data.csv file has been closed.");
    }
}