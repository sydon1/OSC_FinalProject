//
// Created by sodir on 12/25/24.
//

#include "datamgr.h"

#include <string.h>

#include "config.h"
#include "sbuffer.h"

#define LINE_BUFFER_SIZE 12

dplist_t *list;

void *data_manager(void *args) {
    datamgr_args_t *params = (datamgr_args_t*)args;
    dplist_t *sensor_list;
    sensor_data_t data;

    // Initialize sensor list and parse mapping file
    sensor_list = dpl_create(element_copy, element_free, element_compare);
    if (parse_sensor_map(sensor_list) != 0) {
        write_to_log_process("Failed to parse sensor mapping file");
        dpl_free(&sensor_list, true);
        return NULL;
    }

    write_to_log_process("Data manager initialized");

    // Process data from buffer
    while (1) {
        int result = sbuffer_read(params->buffer, &data, 1);  // Stage 1 = data manager

        if (result == SBUFFER_NO_DATA) {
            break;  // End marker received
        }

        if (result == SBUFFER_SUCCESS) {
            process_sensor_data(sensor_list, &data);
        }
    }

    // Cleanup
    dpl_free(&sensor_list, true);
    write_to_log_process("Data manager shutting down");
    return NULL;
}

int parse_sensor_map(dplist_t *list) {
    FILE *fp = fopen("room_sensor.map", "r");
    if (!fp) return -1;

    sensor_data_elem_t sensor;
    uint16_t room_id;
    uint16_t sensor_id;

    while (fscanf(fp, "%hu %hu", &room_id, &sensor_id) == 2) {
        sensor.sensor_id = sensor_id;
        sensor.room_id = room_id;
        sensor.current_idx = 0;
        memset(sensor.running_avg, 0, sizeof(sensor.running_avg));
        dpl_insert_at_index(list, &sensor, 0, true);
    }

    fclose(fp);
    return 0;
}

void process_sensor_data(dplist_t *list, sensor_data_t *data) {
    sensor_data_elem_t dummy = {.sensor_id = data->id};
    int idx = dpl_get_index_of_element(list, &dummy);

    if (idx == -1) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Received sensor data with invalid sensor node ID %d", data->id);
        write_to_log_process(msg);
        return;
    }

    sensor_data_elem_t *sensor = dpl_get_element_at_index(list, idx);
    sensor->running_avg[sensor->current_idx] = data->value;
    sensor->current_idx = (sensor->current_idx + 1) % RUN_AVG_LENGTH;
    sensor->last_modified = data->ts;

    check_sensor_limits(sensor);
}

void check_sensor_limits(sensor_data_elem_t *sensor) {
    double sum = 0;
    int valid_readings = 0;

    for (int i = 0; i < RUN_AVG_LENGTH; i++) {
        if (sensor->running_avg[i] != 0) {
            sum += sensor->running_avg[i];
            valid_readings++;
        }
    }

    if (valid_readings == 0) return;

    double avg = sum / valid_readings;
    char msg[128];

    if (avg > SET_MAX_TEMP) {
        snprintf(msg, sizeof(msg), "Sensor node %d reports it's too hot (avg temp = %.1f)",
                sensor->sensor_id, avg);
        write_to_log_process(msg);
    } else if (avg < SET_MIN_TEMP) {
        snprintf(msg, sizeof(msg), "Sensor node %d reports it's too cold (avg temp = %.1f)",
                sensor->sensor_id, avg);
        write_to_log_process(msg);
    }
}

void datamgr_free() {
    dpl_free(&list, true);
}

uint16_t datamgr_get_room_id(sensor_id_t sensor_id) {
    uint16_t room = 0;
    sensor_data_t temp = {0};
    temp.id = sensor_id;

    int index = dpl_get_index_of_element(list, &temp);
    if (index == -1) {
        fprintf(stderr, "Sensor with that ID not in list\n");
    }

    sensor_data_element *node = dpl_get_element_at_index(list, index); //node moet niet gefreed worden want hij point naar iets dat al bestaat
    room = node->room_id;

    return room;
}

sensor_value_t datamgr_get_avg(sensor_id_t sensor_id) {
    double avg = 0;
    double sum = 0;
    sensor_data_t temp = {0};
    temp.id = sensor_id;

    int index = dpl_get_index_of_element(list, &temp);

    if (index == -1) {
        fprintf(stderr, "Sensor with that ID not in list\n");
    }

    sensor_data_element *node = dpl_get_element_at_index(list, index);

    for(int i = 0; i < RUN_AVG_LENGTH; i++) {
        sum += node->running_avg[i];
    }
    avg = sum / RUN_AVG_LENGTH;

    char log_msg[100];
    if(avg > SET_MAX_TEMP) {
        snprintf(log_msg, sizeof(log_msg), "Sensor node %d reports it's too hot (avg temp = %.1f)", node->sensor_id, avg);
        write_to_log_process(log_msg);
    } else if(avg < SET_MIN_TEMP) {
        snprintf(log_msg, sizeof(log_msg), "Sensor node %d reports it's too cold (avg temp = %.1f)", node->sensor_id, avg);
        write_to_log_process(log_msg);
    }

    return avg;
}

time_t datamgr_get_last_modified(sensor_id_t sensor_id) {
    time_t timestamp = 0;
    sensor_data_t temp = {0};
    temp.id = sensor_id;

    int index = dpl_get_index_of_element(list, &temp);

    if (index == -1) {
        fprintf(stderr, "Sensor with that ID not in list\n");
    }

    sensor_data_element *node = dpl_get_element_at_index(list, index);
    timestamp = node->last_modified;

    return timestamp;
}


int datamgr_get_total_sensors() {
    return dpl_size(list);
}

void* element_copy(void* element) {
    sensor_data_element* copy = malloc(sizeof(sensor_data_element));
    memcpy(copy, element, sizeof(sensor_data_element));
    return copy;
}

void element_free(void** element) {
    free(*element);
    *element = NULL;
}

int element_compare(void* x, void* y) {
    sensor_data_element* element_x = (sensor_data_element*)x;
    sensor_data_element* element_y = (sensor_data_element*)y;
    return element_x->sensor_id - element_y->sensor_id;
}