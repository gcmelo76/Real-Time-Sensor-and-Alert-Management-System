/*

Joao Pedro Morais Gaspar          nº2021221276
Guilherme Craveiro de Melo     nº 2021217138

*/
#include "mainFunctions.h"
#include "system_manager.h"

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

void setupSignals() {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

void setupLogFile() {
    logFile = fopen(LOG_FILE,"w");
    if(logFile == NULL){
        char errorMsg[256];
        sprintf(errorMsg, "Error opening logFile: %s", strerror(errno));
        outputToConsole(0, errorMsg);
        exit(0);
    }
    time(&t);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%c", localtime(&t));
    fprintf(logFile,"%s HOME_IOT SIMULATOR STARTING\n", timeStr);
    fflush(logFile);
}

void verifyArguments(int argc, char const *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s {config_file}\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("configFile error");
        exit(EXIT_FAILURE);
    }
    fclose(file);
}

void setupConfigFile(char const *argv[]) {
    char buffer[11];
    int config_values[5];
    int index = 0;

    configFile = fopen(argv[1], "r");
    if (configFile == NULL) {
        fprintf(stderr, "Unable to open the config file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    while (fgets(buffer, sizeof(buffer), configFile) != NULL && index < 5) {
        int value = atoi(buffer);
        if ((value < 1 && index < 4) || value < 0) {
            fprintf(stderr, "Invalid parameter values in the config file!\n");
            exit(EXIT_FAILURE);
        }
        config_values[index++] = value;
    }

    if (index < 5) {
        fprintf(stderr, "Insufficient parameters in the config file!\n");
        exit(EXIT_FAILURE);
    }

    memcpy(system_config, config_values, sizeof(config_values));
    pipe_array = malloc(system_config[1] * sizeof(int[2]));

    if (pipe_array == NULL) {
        fprintf(stderr, "Failed to allocate memory for pipe_array: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    fclose(configFile);
}

void printErrorAndExit(const char *message) {
    time_t current_time = time(NULL);
    char *time_str = ctime(&current_time);
    fprintf(stderr, "%s\n", message);
    if (logFile != NULL) {
        fprintf(logFile,"%s %s\n", time_str, message);
        fclose(logFile);
    }
    exit(EXIT_FAILURE);
}

void createSharedMemory() {
    size_t total_size = sizeof(memory_struct) +
                        system_config[1] * sizeof(bool) +
                        system_config[2] * sizeof(sensor_info) +
                        system_config[3] * sizeof(sensor_data) +
                        system_config[4] * sizeof(alert_info);

    shared_memory_id = shmget(IPC_PRIVATE, total_size, IPC_CREAT | 0700);
    if (shared_memory_id == -1) {
        perror("Error in shmget");
        exit(EXIT_FAILURE);
    }

    #ifdef DEBUG
    printf("Shared memory created with ID: %d\n", shared_memory_id);
    #endif
}

void attachSharedMemory() {
    shared_memory = (memory_struct*) shmat(shared_memory_id, NULL, 0);
    if (shared_memory == (memory_struct*) -1) {
        perror("Error in shmat");
        exit(EXIT_FAILURE);
    }

    #ifdef DEBUG
    printf("Shared memory attached at address: %p\n", shared_memory);
    #endif
    shared_memory->worker_status_array = (bool*) (shared_memory + 1);
    shared_memory->sensor_stats_array =
        (sensor_info*) (shared_memory->worker_status_array + system_config[1]);
    shared_memory->sensors_array =
        (sensor_data*) (shared_memory->sensor_stats_array + system_config[2]);
    shared_memory->alerts_array =
        (alert_info*) (shared_memory->sensors_array + system_config[3]);

    shared_memory->sensor_stat_count = 0;
    shared_memory->sensor_total = 0;
    shared_memory->alert_total = 0;
}

void setupKeyAndMessageQueue() {
    key_t key = ftok("user_console.c",'B');
    if (key == -1) {
        perror("Error on ftok key generation");
        fprintf(logFile,"%s Error on ftok key generation: %s\n", ctime(&t), strerror(errno));
        shmctl(shared_memory_id, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }
    message_queue_id = msgget(key, IPC_CREAT | 0700);
    if (message_queue_id == -1) {
        perror("Error: Cannot get message queue");
        fprintf(logFile,"%s Error: Cannot get message queue: %s\n", ctime(&t), strerror(errno));
        shmctl(shared_memory_id, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }
    #ifdef DEBUG
    printf("Message queue created with id: %d\n", message_queue_id);
    #endif
}


void setupSemaphores() {
    sem_unlink("AVAILABLE_WORKERS_SEMAPHORE");
    available_workers_semaphore = sem_open("AVAILABLE_WORKERS_SEMAPHORE",O_CREAT|O_EXCL,0700,system_config[1]);
    sem_unlink("SHARED_MEMORY_MUTEX");
    shared_memory_mutex =sem_open("SHARED_MEMORY_MUTEX",O_CREAT|O_EXCL,0700,1);
    sem_unlink("OUTPUT_MUTEX");
    output_mutex =sem_open("OUTPUT_MUTEX",O_CREAT|O_EXCL,0700,1);
    sem_unlink("SENSOR_STAT_MUTEX");
    sensor_stat_mutex =sem_open("SENSOR_STAT_MUTEX",O_CREAT|O_EXCL,0700,1);
}

void setupSigaction() {
    new_action.sa_handler = signalHandler;
    new_action.sa_flags = 0;
    sigfillset(&new_action.sa_mask);
    sigaction(SIGINT,&new_action,NULL);
}

void setupInternalQueue() {
    current_queue_size = 0;
    head = (INTERNAL_LIST_HEAD*)malloc(sizeof(INTERNAL_LIST_HEAD));
    head->FIRST_NODE = NULL;
    head->LAST_NODE = NULL;
}

void setupChildProcess(int i) {
    sigaction(SIGTERM, &new_action, NULL);
    worker_number = i;
    global_pipe = pipe_array[i][0];
    output_mutex = sem_open("OUTPUT_MUTEX", 0);
    shared_memory_mutex = sem_open("SHARED_MEMORY_MUTEX", 0);
    available_workers_semaphore = sem_open("AVAILABLE_WORKERS_SEMAPHORE", 0);
    process(pipe_array[i]);
}

void setupAlertWatcherProcess() {
    sigaction(SIGTERM, &new_action, NULL);
    alertsWatcher();
}

void handleWorkerCreationError() {
    char temp_s[100];
    sprintf(temp_s, "Erro worker creation: %s", strerror(errno));
    outputToConsole(0, temp_s);
    cleanUp();
}

void handleAlertWatcherCreationError() {
    char temp_s[100];
    sprintf(temp_s, "Erro alert_watcher creation: %s", strerror(errno));
    outputToConsole(0, temp_s);
    cleanUp();
}

void spawnProcesses() {
    __pid_t pid;
    main_process_identifier = getpid();

    int i = 0;
    while (i < system_config[1]) {
        pipe(pipe_array[i]);

        pid = fork();
        if (pid == 0) { 
            setupChildProcess(i);
            exit(0);
        } else if (pid == -1) {
            handleWorkerCreationError();
        }

        close(pipe_array[i][0]);
        i++;
    }

    alert_watcher_process_identifier = fork();
    if (alert_watcher_process_identifier == 0) {
        setupAlertWatcherProcess();
        exit(0);
    } else if (alert_watcher_process_identifier == -1) {
        handleAlertWatcherCreationError();
    }
}

int createNamedPipe(const char* pipeName) {
    if ((mkfifo(pipeName, O_CREAT | O_EXCL | 0600) < 0) && (errno != EEXIST)) {
        time(&t);
        perror("Cannot create named pipe");
        fprintf(logFile, "%s Cannot create named pipe '%s': %s\n", ctime(&t), pipeName, strerror(errno));
        fclose(logFile);
        fclose(configFile);
        exit(0);
    }
    return 0;
}

void handlePipeCreationError(const char* pipeName) {
    time(&t);
    perror("Cannot create named pipe");
    fprintf(logFile, "%s Cannot create named pipe '%s': %s\n", ctime(&t), pipeName, strerror(errno));
    fclose(logFile);
    fclose(configFile);
    exit(0);
}

void setupPipes() {
    if (createNamedPipe(CONSOLE_PIPE) < 0) {
        handlePipeCreationError(CONSOLE_PIPE);
    }
    if (createNamedPipe(SENSOR_PIPE) < 0) {
        handlePipeCreationError(SENSOR_PIPE);
    }
}

int createThread(pthread_t* thread, void* (*start_routine)(void*), void* arg) {
    int return_value = pthread_create(thread, NULL, start_routine, arg);
    return return_value;
}

void handleThreadCreationError(int error_code) {
    outputToConsole(0, strerror(error_code));
}

void createThreads() {
    int return_value;
    if ((return_value = createThread(&console_thread, console,NULL)) != 0) {
        handleThreadCreationError(return_value);
    }
    if ((return_value = createThread(&sensor_thread, sensor,NULL)) != 0) {
        handleThreadCreationError(return_value);
    }
    if ((return_value = createThread(&dispatcher_thread, dispatch, pipe_array)) != 0) {
        handleThreadCreationError(return_value);
    }
}

void closePipesAndKillProcesses() {
    for (int i = 0; i < system_config[1]; i++){
        close(pipe_array[i][1]);
    }
}

int unlinkNamedPipe(const char* pipeName) {
    int return_value = unlink(pipeName);
    return return_value;
}

int closeFileDescriptor(int fileDescriptor) {
    int return_value = close(fileDescriptor);
    return return_value;
}

int detachSharedMemory(void* sharedMemory) {
    int return_value = shmdt(sharedMemory);
    return return_value;
}

void handleUnlinkError(const char* name) {
    outputToConsole(0, strerror(errno));
}

void handleCloseError(int fileDescriptor) {
    outputToConsole(0, strerror(errno));
}

void handleDetachError(void* sharedMemory) {
    outputToConsole(0, strerror(errno));
}

void unlinkAndCloseResources() {
    int return_value;

    return_value = unlinkNamedPipe(CONSOLE_PIPE);
    if (return_value == -1) {
        handleUnlinkError(CONSOLE_PIPE);
    }

    return_value = closeFileDescriptor(console_file_descriptor);
    if (return_value == -1) {
        handleCloseError(console_file_descriptor);
    }

    return_value = unlinkNamedPipe(SENSOR_PIPE);
    if (return_value == -1) {
        handleUnlinkError(SENSOR_PIPE);
    }

    return_value = closeFileDescriptor(sensor_file_descriptor);
    if (return_value == -1) {
        handleCloseError(sensor_file_descriptor);
    }

    return_value = detachSharedMemory(shared_memory);
    if (return_value == -1) {
        handleDetachError(shared_memory);
    }
}


void waitForAllProcessesToFinish() {
    while(waitpid((pid_t)-1,NULL,0)>0);
}

int unlinkSemaphore(const char* semaphoreName) {
    int return_value = sem_unlink(semaphoreName);
    return return_value;
}


void unlinkSemaphores() {
    int return_value;
    return_value = unlinkSemaphore("OUTPUT_MUTEX");
    if (return_value == -1) {
        handleUnlinkError("OUTPUT_MUTEX");
    }

    return_value = unlinkSemaphore("SHARED_MEMORY_MUTEX");
    if (return_value == -1) {
        handleUnlinkError("SHARED_MEMORY_MUTEX");
    }

    return_value = unlinkSemaphore("AVAILABLE_WORKERS_SEMAPHORE");
    if (return_value == -1) {
        handleUnlinkError("AVAILABLE_WORKERS_SEMAPHORE");
    }
}


void removeIPCResources() {
    int return_value = sem_unlink("SENSOR_STAT_MUTEX");
    if(return_value==-1){
        printf("%s",strerror(errno));
    }
    return_value = msgctl(message_queue_id, IPC_RMID, NULL);
    if(return_value==-1){
        printf("%s",strerror(errno));
    }
    return_value = shmctl(shared_memory_id, IPC_RMID, NULL);
    if(return_value==-1){
        outputToConsole(0,strerror(errno));
    }
    printf("%s\n","Cleaning INTERNAL_LIST");
    free(head);
}


void cleanUp() {
    unlinkAndCloseResources();
    waitForAllProcessesToFinish();
    unlinkSemaphores();
    removeIPCResources();
}

