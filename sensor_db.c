//
// Created by sodir on 12/25/24.
//

#include "sensor_db.h"
#include "sensor_db.h"

#include <stdbool.h>
#include <stdio.h>

#include "config.h"
#include "sbuffer.h"


#include "sensor_db.h"
#include <string.h>

void *storage_manager(void *args) {
    storage_args_t *params = (storage_args_t*)args;
    sensor_data_t data;
    char log_message[128];

    // Open database file
    FILE *fp = open_db("data.csv");
    if (!fp) {
        write_to_log_process("Failed to open data file");
        return NULL;
    }

    write_to_log_process("A new data.csv file has been created.");

    // Process data from buffer
    while (1) {
        int result = sbuffer_read(params->buffer, &data, 2);  // Stage 2 = storage manager

        if (result == SBUFFER_NO_DATA) {
            break;  // End marker received
        }

        if (result == SBUFFER_SUCCESS) {
            if (write_sensor_data(fp, &data) == 0) {
                snprintf(log_message, sizeof(log_message),
                        "Data insertion from sensor %d succeeded", data.id);
                write_to_log_process(log_message);
            } else {
                write_to_log_process("Failed to write sensor data");
            }
        }

        // After successfully reading and writing, remove from buffer
        sbuffer_remove(params->buffer, &data);
    }

    // Cleanup
    close_db(fp);
    write_to_log_process("Storage manager shutting down");
    return NULL;
}

FILE *open_db(const char *filename) {
    FILE *fp = fopen(filename, "w");  // Create new file or truncate existing
    if (!fp) {
        write_to_log_process("Could not open data file");
        return NULL;
    }
    return fp;
}

int write_sensor_data(FILE *fp, sensor_data_t *data) {
    if (!fp || !data) return -1;

    int result = fprintf(fp, "%d,%.2f,%ld\n",
                        data->id, data->value, data->ts);

    if (result < 0) return -1;

    fflush(fp);
    return 0;
}

void close_db(FILE *fp) {
    if (fp) {
        write_to_log_process("The data.csv file has been closed.");
        fclose(fp);
    }
}