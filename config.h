/**
 * \author {AUTHOR}
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <time.h>
#include "lib/tcpsock.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>



/* definitons */
#define LOG_FILE "gateway.log"
#define DATA_FILE_NAME "data.csv"
#define MAP_FILE "room_sensor.map"

#ifndef TIMEOUT
#define TIMEOUT 5  // Connection timeout in seconds
#endif

#ifndef SET_MIN_TEMP
#define SET_MIN_TEMP 10
#endif

#ifndef SET_MAX_TEMP
#define SET_MAX_TEMP 20
#endif

#define LOG_MSG_MAX_LEN 300
#define RUN_AVG_LENGTH 5

/* Typedef */
typedef uint16_t sensor_id_t;
typedef double sensor_value_t;
typedef time_t sensor_ts_t;

/* sensor data*/
typedef struct sensor_data_t {
   sensor_id_t id;
   sensor_value_t value;
   sensor_ts_t ts;
} sensor_data_t;

typedef enum processing_stage processing_stage_t;
/* Component Parameters */
typedef enum processing_stage {
    STAGE_NEW = 0,
    STAGE_DATA_PROCESSED = 1,
    STAGE_STORAGE_PROCESSED = 2
 }processing_stage_t;

typedef enum {
    SUCCESS = 0,
    ERR_FILE_IO = -1,
    ERR_PIPE = -2,
    ERR_FORK = -3,
    ERR_MEMORY = -4,
    ERR_THREAD = -5
 } status_t;

typedef struct sbuffer sbuffer_t;
typedef struct connmgrParam {
   int max_con;
   int port;
   sbuffer_t* sBuffer;
} connmgrParam_t;

typedef struct connParam {
   tcpsock_t *client;
   sbuffer_t* sBuffer;
   int conn_id;
} connParam_t;

typedef struct {
   void* shared_buffer;
} shared_buffer_datamgr_t;

typedef struct {
   sensor_id_t sensor_id;
   uint16_t room_id;
   double running_avg[RUN_AVG_LENGTH];
   int index;
   time_t last_modified;
} sensor_data_element;

/* Function Declarations that are used in main*/
/**
 * Writes a message to the logging process through the pipe
 * @param msg Null-terminated string message to log (max LOG_MSG_MAX_LEN chars)
 * @return status_t SUCCESS on successful write, error code otherwise
 */
int write_to_log_process(char *msg);

/**
 * Creates and initializes the logging child process
 * @return status_t SUCCESS if process created, error code otherwise
 */
int create_log_process();

/**
 * Terminates the logging process
 * @return status_t SUCCESS if terminated cleanly, error code otherwise
 */
int end_log_process();

#endif /* _CONFIG_H_ */
