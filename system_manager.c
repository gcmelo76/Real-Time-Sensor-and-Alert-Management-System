/*

Joao Pedro Morais Gaspar          nº2021221276
Guilherme Craveiro de Melo     nº 2021217138

*/
#include <stdio.h>
#include <stdlib.h>
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

#include "system_manager.h"
#include "otherFunctions.h"
#include "mainFunctions.h"

#define SENSOR_PIPE "SENSOR_PIPE"
#define CONSOLE_PIPE "CONSOLE_PIPE"
#define LOG_FILE "log.txt"
#define DEBUG


int console_file_descriptor,sensor_file_descriptor,current_queue_size,shared_memory_id,message_queue_id;
int system_config[5],global_pipe,worker_number;
int shared_memory_id, message_queue_id;

struct sigaction new_action;
INTERNAL_LIST_HEAD* head;
memory_struct* shared_memory;

sem_t *available_workers_semaphore,*shared_memory_mutex,*output_mutex,*sensor_stat_mutex;

pid_t main_process_identifier,alert_watcher_process_identifier;
pthread_t sensor_thread,console_thread,dispatcher_thread;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_full_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t queue_empty_condition = PTHREAD_COND_INITIALIZER;

FILE *logFile,*configFile;

time_t t;

int (*pipe_array)[2];

char temp_storage[123];

int main(int argc, char const *argv[]){
    
    setupSignals();
    setupLogFile();
    verifyArguments(argc, argv);
    setupConfigFile(argv);
    createSharedMemory();
    attachSharedMemory();

    setupSemaphores();
    setupSigaction();
    spawnProcesses();
    setupPipes();
    setupInternalQueue();
    createThreads();
    
    pthread_join(console_thread,NULL);
    pthread_join(sensor_thread,NULL);
    pthread_join(dispatcher_thread,NULL);
    
    closePipesAndKillProcesses();

    cleanUp();

    return 0;
}
