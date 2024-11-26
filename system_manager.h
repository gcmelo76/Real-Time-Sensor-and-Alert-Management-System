/*

Joao Pedro Morais Gaspar          nº2021221276
Guilherme Craveiro de Melo     nº 2021217138

*/
#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#define SENSOR_PIPE "SENSOR_PIPE"
#define CONSOLE_PIPE "CONSOLE_PIPE"
#define LOG_FILE "log.txt"
#define DEBUG

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <sys/msg.h>

typedef struct INTERNAL_LIST{
    char message[125];
    int priority_level;
    struct INTERNAL_LIST *NEXT_NODE;
}INTERNAL_LIST;

typedef struct INTERNAL_LIST_HEAD{
    INTERNAL_LIST *FIRST_NODE;
    INTERNAL_LIST *LAST_NODE;
}INTERNAL_LIST_HEAD;

typedef struct sensor_data{
    char sensor_id[32];
}sensor_data;

typedef struct sensor_info{
    bool alert_status;
    char sensor_key[32];
    int last_val,min_val,max_val,update_count;
    double avg_val;
}sensor_info;

typedef struct alert_info{
    int min_val,max_val,user_console_identifier;
    char sensor_id[32];
    char sensor_key[32];
}alert_info;

typedef struct{
    int sensor_stat_count,sensor_total,alert_total;
    bool *worker_status_array; 
    sensor_info *sensor_stats_array;
    sensor_data *sensors_array;
    alert_info *alerts_array;
} memory_struct;

typedef struct {
    long message_type;
    char message_content[88];
}Message;

extern int console_file_descriptor,sensor_file_descriptor,current_queue_size,shared_memory_id,message_queue_id;
extern int system_config[5],global_pipe,worker_number;
extern int shared_memory_id, message_queue_id;

extern struct sigaction new_action;
extern INTERNAL_LIST_HEAD* head;
extern memory_struct* shared_memory;

extern sem_t *available_workers_semaphore,*shared_memory_mutex,*output_mutex,*sensor_stat_mutex;

extern pid_t main_process_identifier,alert_watcher_process_identifier;
extern pthread_t sensor_thread,console_thread,dispatcher_thread;
extern pthread_mutex_t queue_lock;
extern pthread_cond_t queue_full_condition;
extern pthread_cond_t queue_empty_condition;

extern FILE *logFile,*configFile;

extern time_t t;

extern int (*pipe_array)[2];

extern char temp_storage[123];

#endif
