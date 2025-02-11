//
// Created by sodir on 12/9/24.
//

#include "datamgr.h"
#include "sbuffer.h"
#include "config.h"

#define LINE_BUFFER_SIZE 12

dplist_t *list;

void *data_manager(void *args) {
    datamanager_arguments_t *params = (datamanager_arguments_t*)args;
    dplist_t *sensor_list = NULL;
    sensor_data_t data;
    bool running = true;

    //init sensor list
    sensor_list = dpl_create(element_copy, element_free, element_compare);
    if (!sensor_list) {
        write_to_log_process("Failed to create sensor list");
        return NULL;
    }

    //parse sensor mappings
    if (parse_sensor_map(sensor_list) != 0) {
        write_to_log_process("Failed to parse sensor mapping file");
        dpl_free(&sensor_list, true);
        return NULL;
    }

    write_to_log_process("Data manager initialized");
    while (running) {
        int result = sbuffer_read(params->sBuffer, &data, 1);  // Stage 1 = data manager

        if (result == SBUFFER_NO_DATA) {
            running = false;
        } else if (result == SBUFFER_SUCCESS) {
            process_sensor_data(sensor_list, &data);
        } else {
            write_to_log_process("Error reading from buffer in data manager");
            running = false;
        }
    }
    write_to_log_process("Data manager processing complete");
    datamgr_cleanup(sensor_list);
    write_to_log_process("Data manager shutting down");

    return NULL;
}

void datamgr_cleanup(dplist_t *list) {
    if (list) {
        dpl_free(&list, true);
    }
}

int parse_sensor_map(dplist_t *list) {
    FILE *fp_map = fopen(MAP_FILE, "r");
    if (!fp_map) {
      write_to_log_process("Failed to open room_sensor.map");
      return -1;
    }

    sensor_data_element_t sensor;
    uint16_t room_id;
    uint16_t sensor_id;

    while (fscanf(fp_map, "%hu %hu", &room_id, &sensor_id) == 2) {
        sensor.sensor_id = sensor_id;
        sensor.room_id = room_id;
        sensor.current_index = 0;
        memset(sensor.running_avg, 0, sizeof(sensor.running_avg));
        dpl_insert_at_index(list, &sensor, 0, true);
    }

    fclose(fp_map);
    return 0;
}

void process_sensor_data(dplist_t *list, sensor_data_t *data) {
    if (!list || !data) return;

    sensor_data_element_t dummy = {.sensor_id = data->id};
    int index = dpl_get_index_of_element(list, &dummy);

    if (index == -1) {
        char log_message[300];
        snprintf(log_message, sizeof(log_message), "Received sensor data with invalid sensor node ID %d", data->id);
        write_to_log_process(log_message);
        return;
    }

    sensor_data_element_t *sensor = dpl_get_element_at_index(list, index);
    if (!sensor) return;

    sensor->running_avg[sensor->current_index] = data->value;
    sensor->current_index = (sensor->current_index + 1) % RUN_AVG_LENGTH;
    sensor->last_modified = data->ts;

    check_sensor_limits(sensor);
}

void check_sensor_limits(sensor_data_element_t *sensor) {
    if (!sensor) return;

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
    char log_message[300];

    if (avg > SET_MAX_TEMP) {
        snprintf(log_message, sizeof(log_message), "Sensor node %d reports it's too hot (avg temp = %.1f)", sensor->sensor_id, avg);
        write_to_log_process(log_message);
    } else if (avg < SET_MIN_TEMP) {
        snprintf(log_message, sizeof(log_message),"Sensor node %d reports it's too cold (avg temp = %.1f)", sensor->sensor_id, avg);
        write_to_log_process(log_message);
    }
}

uint16_t datamgr_get_room_id(sensor_id_t sensor_id) {
    uint16_t room = 0;
    sensor_data_t temp = {0};
    temp.id = sensor_id;

    int index = dpl_get_index_of_element(list, &temp);
    if (index == -1) {
        fprintf(stderr, "Sensor with that ID not in list\n");
    }

    sensor_data_element_t *node = dpl_get_element_at_index(list, index); //node moet niet gefreed worden want hij point naar iets dat al bestaat
    room = node->room_id;

    return room;
}

time_t datamgr_get_last_modified(sensor_id_t sensor_id) {
    time_t timestamp = 0;
    sensor_data_t temp = {0};
    temp.id = sensor_id;

    int index = dpl_get_index_of_element(list, &temp);

    if (index == -1) {
        fprintf(stderr, "Sensor with that ID not in list\n");
    }

    sensor_data_element_t *node = dpl_get_element_at_index(list, index);
    timestamp = node->last_modified;

    return timestamp;
}


int datamgr_get_total_sensors() {
    return dpl_size(list);
}

void* element_copy(void* element) {
    sensor_data_element_t* copy = malloc(sizeof(sensor_data_element_t));
    memcpy(copy, element, sizeof(sensor_data_element_t));
    return copy;
}

void element_free(void** element) {
    free(*element);
    *element = NULL;
}

int element_compare(void* x, void* y) {
    sensor_data_element_t* element_x = (sensor_data_element_t*)x;
    sensor_data_element_t* element_y = (sensor_data_element_t*)y;
    return element_x->sensor_id - element_y->sensor_id;
}