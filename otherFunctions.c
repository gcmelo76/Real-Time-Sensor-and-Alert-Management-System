/*

Joao Pedro Morais Gaspar          nº2021221276
Guilherme Craveiro de Melo     nº 2021217138

*/

#include "otherFunctions.h"

#include "system_manager.h"

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

bool show = false;

void outputToConsole(short int function,char s[]){
    sem_wait(output_mutex);
    if(function == 0 || function == 2){
        printf("%s\n",s);
    }
    if(function == 0 || function == 1){
        time(&t);
        fprintf(logFile,"%s %s\n",ctime(&t),s);
        fflush(logFile);
    }
    sem_post(output_mutex);
}

void signalHandler(int signo){
    pthread_t current_thread = pthread_self();
    pid_t current_process = getpid();
    int return_value;
    if(signo==SIGINT){
        if(current_thread == console_thread || current_thread == sensor_thread || current_thread == dispatcher_thread){
            return;
        }
        outputToConsole(0,"SIGINT recived");
        outputToConsole(0,"HOME_IOT is going to close");
        return_value = pthread_cancel(console_thread);
        if(return_value!=0){
            outputToConsole(0,strerror(return_value));
        }
        return_value = pthread_cancel(sensor_thread);
        if(return_value!=0){
            outputToConsole(0,strerror(return_value));
        }
        return_value = pthread_cancel(dispatcher_thread);
        if(return_value!=0){
            outputToConsole(0,strerror(return_value));
        }
        outputToConsole(0,"All threads closed");
        pthread_mutex_destroy(&queue_lock);
        pthread_cond_destroy(&queue_full_condition);
        pthread_cond_destroy(&queue_empty_condition);
    }
    else{
        char s[42];
        if (current_process==alert_watcher_process_identifier){
            outputToConsole(0,"alertsWatcher exiting");
            cleanUp();
            return;
        }
        sem_wait(shared_memory_mutex);
        if(!*(shared_memory->worker_status_array+worker_number)){
            sprintf(s,"Worker %d exiting",worker_number);
            outputToConsole(0,s);
            return_value = close(global_pipe);
            if(return_value==-1){
                outputToConsole(0,strerror(errno));
            }
        }
        else{
            sprintf(s,"Waiting for worker %d to finish",worker_number);
            outputToConsole(0,s);
        }
        sem_post(shared_memory_mutex);
    }
}


INTERNAL_LIST* createNewNode(short int priority_level, char message_content[]) {
    INTERNAL_LIST *new_node = (INTERNAL_LIST*) malloc(sizeof(INTERNAL_LIST));
    strcpy(new_node->message, message_content);
    new_node->priority_level = priority_level;
    new_node->NEXT_NODE = NULL;
    return new_node;
}

INTERNAL_LIST* findInsertionPoint() {
    INTERNAL_LIST *current_node = head->FIRST_NODE;
    INTERNAL_LIST *previous_node = head->FIRST_NODE;

    while (current_node != NULL && current_node->priority_level > 0) {
        previous_node = current_node;
        current_node = current_node->NEXT_NODE;
    }

    if (current_node == previous_node) {
        return NULL;
    } else {
        return previous_node;
    }
}

void insertNode(INTERNAL_LIST *insertion_point, INTERNAL_LIST *new_node) {
    if (insertion_point == NULL) {
        new_node->NEXT_NODE = head->FIRST_NODE;
        head->FIRST_NODE = new_node;
    } else {
        new_node->NEXT_NODE = insertion_point->NEXT_NODE;
        insertion_point->NEXT_NODE = new_node;
    }
}

void addToQueue(short int priority_level, char message_content[]) {
    pthread_mutex_lock(&queue_lock);
    if (current_queue_size >= system_config[0] && priority_level <= 0) {
        sprintf(temp_storage, "QUEUE IS FULL:%s discarded", message_content);
        outputToConsole(0, temp_storage);
    } else {
        while (current_queue_size >= system_config[0]) {
            #ifdef DEBUG
            outputToConsole(2, "Priority message_content waiting for queue to free 1 space");
            #endif
            pthread_cond_wait(&queue_full_condition, &queue_lock);
            #ifdef DEBUG
            outputToConsole(2, "Priority message_content waited and now will proceed");
            #endif
        }
        INTERNAL_LIST *new_node = createNewNode(priority_level, message_content);
        if (head->FIRST_NODE == NULL) {
            head->FIRST_NODE = new_node;
            head->LAST_NODE = new_node;
        } else {
            if (!priority_level) {
                head->LAST_NODE->NEXT_NODE = new_node;
                head->LAST_NODE = new_node;
            } else {
                INTERNAL_LIST *insertion_point = findInsertionPoint(priority_level);
                insertNode(insertion_point, new_node);
            }
        }
        current_queue_size++;
    }
    pthread_cond_signal(&queue_empty_condition);
    pthread_mutex_unlock(&queue_lock);
}



int popFromQueue(char dest_string[]){
    if (head->FIRST_NODE == NULL) {
        return 0;
    } else {
        INTERNAL_LIST *ptr = head->FIRST_NODE;
        strcpy(dest_string, ptr->message);
        head->FIRST_NODE = ptr->NEXT_NODE;
        if(head->FIRST_NODE == NULL){
            head->LAST_NODE = NULL;
        }
        current_queue_size--;
        free(ptr);
        return 1;
    }
}


bool findSensorById(char* sensor_id) {
    for (int i = 0; i < shared_memory->sensor_total; i++) {
        if (strcmp(sensor_id, (shared_memory->sensors_array + i)->sensor_id) == 0) {
            return true;
        }
    }
    return false;
}

void addNewSensor(char* sensor_id) {
    strcpy((shared_memory->sensors_array + shared_memory->sensor_total)->sensor_id, sensor_id);
    shared_memory->sensor_total++;
}

bool findSensorStatsByKey(char* sensor_key) {
    for (int i = 0; i < shared_memory->sensor_stat_count; i++) {
        if (strcmp(sensor_key, (shared_memory->sensor_stats_array + i)->sensor_key) == 0) {
            return true;
        }
    }
    return false;
}

void updateSensorStats(char* sensor_key, int value) {
    sensor_info* Pointer = getSensorStatsByKey(sensor_key);
    Pointer->avg_val = (Pointer->update_count * Pointer->avg_val + value) / (Pointer->update_count + 1);
    Pointer->last_val = value;
    Pointer->update_count++;
    Pointer->max_val = Pointer->max_val < value ? value : Pointer->max_val;
    Pointer->min_val = Pointer->min_val > value ? value : Pointer->min_val;
    Pointer->alert_status = false;
}

void addNewSensorStats(char* sensor_key, int value) {
    sensor_info* Pointer = shared_memory->sensor_stats_array + shared_memory->sensor_stat_count;
    strcpy(Pointer->sensor_key, sensor_key);
    Pointer->avg_val = Pointer->last_val = Pointer->max_val = Pointer->min_val = value;
    Pointer->update_count = 1;
    shared_memory->sensor_stat_count++;
    Pointer->alert_status = false;
}

sensor_info* getSensorStatsByKey(char* sensor_key) {
    for (int i = 0; i < shared_memory->sensor_stat_count; i++) {
        if (strcmp(sensor_key, (shared_memory->sensor_stats_array + i)->sensor_key) == 0) {
            return shared_memory->sensor_stats_array + i;
        }
    }
    return NULL;
}


void processSensor(char s[]) {
    char *sensor_id, *sensor_key, *sensor_value;
    char Delimiter[] = "#";
    int value;
    bool foundElement = false;
    sensor_id = strtok(s, Delimiter);
    foundElement = findSensorById(sensor_id);

    if (!foundElement) {
        if (shared_memory->sensor_total > system_config[3]) {
            outputToConsole(0, "Sensors list is Full!");
            return;
        }
        addNewSensor(sensor_id);
    }
    sensor_key = strtok(0, Delimiter);
    value = atoi(strtok(0, Delimiter));
    foundElement = findSensorStatsByKey(sensor_key);
    if (foundElement) {
        updateSensorStats(sensor_key, value);
    } else {
        if (shared_memory->sensor_stat_count > system_config[2]) {
            outputToConsole(0, "Stats array is full!");
            return;
        }
        addNewSensorStats(sensor_key, value);
    }
    sem_post(sensor_stat_mutex);
}

void errorSending(char sensor_id[]){
    char temp_storage[300];
    sem_wait(output_mutex);
    sprintf(temp_storage, "Error sending message to %s: %s", sensor_id, strerror(errno));
    outputToConsole(0, temp_storage);
    sem_post(output_mutex);
}

void processStatistics(Message* message, char* sensor_id) {
    sensor_info* Pointer;
    char temp[300];
    bool error = false;
    strcpy(message->message_content, "Key Last Min Max Avg Count");
    if (msgsnd(message_queue_id, message, sizeof(*message) - sizeof(long), 0) == -1) {
        error = true;
    }
    for (int i = 0; i < system_config[2]; i++) {
        if (i >= shared_memory->sensor_stat_count) break;
        Pointer = shared_memory->sensor_stats_array + i;
        snprintf(temp, sizeof(temp), "%s %d %d %d %f %d", Pointer->sensor_key, Pointer->last_val, Pointer->min_val, Pointer->max_val, Pointer->avg_val, Pointer->update_count);
        strcpy(message->message_content, temp);
        if (msgsnd(message_queue_id, message, sizeof(*message) - sizeof(long), 0) == -1) {
            error = true;
            break;
        }
    }
    if (error) {
        errorSending(sensor_id);
    }
}

void resetStatistics(Message* message, char* sensor_id) {
    shared_memory->sensor_total = 0;
    shared_memory->sensor_stat_count = 0;
    strcpy(message->message_content, "OK");
    if (msgsnd(message_queue_id, message, sizeof(*message) - sizeof(long), 0) == -1) {
        errorSending(sensor_id);
    }
}

void processSensorList(Message* message, char* sensor_id) {
    sensor_data *sl;
    strcpy(message->message_content, "ID");
    if (msgsnd(message_queue_id, message, sizeof(*message) - sizeof(long), 0) == -1) {
        errorSending(sensor_id);
    }
    for (int i = 0; i < system_config[3]; i++) {
        sl = shared_memory->sensors_array + i;
        if (i >= shared_memory->sensor_total){
            break;
        }
        sprintf(message->message_content, "%s", sl->sensor_id);
        if (msgsnd(message_queue_id, message, sizeof(*message) - sizeof(long), 0) == -1) {
            errorSending(sensor_id);
        }
    }
}

void createErrorMessage(Message* message, const char* errorCode, const char* sensorId, const char* errorMessage) {
    sprintf(message->message_content, "%s: %s %s", errorCode, sensorId, errorMessage);
}

void createSuccessMessage(Message* message, const char* successMessage) {
    strcpy(message->message_content, successMessage);
}

void addAlert(Message* message, char* sensor_id) {
    alert_info* Alerts = shared_memory->alerts_array;
    char* token = strtok(NULL, "#");
    for (int i = 0; i < shared_memory->alert_total; i++) {
        if (strcmp((Alerts + i)->sensor_id, token) == 0) {
            createErrorMessage(message, "ERROR", token, "alert sensor_id already exists");
            if (msgsnd(message_queue_id, message, sizeof(*message) - sizeof(long), 0) == -1) {
                errorSending(sensor_id);
            }
            return;
        }
    }
    if (shared_memory->alert_total < system_config[4]) {
        Alerts += shared_memory->alert_total;
        strcpy(Alerts->sensor_id, token);
        token = strtok(NULL, "#");
        strcpy(Alerts->sensor_key, token);
        token = strtok(NULL, "#");
        Alerts->min_val = atoi(token);
        token = strtok(NULL, "#");
        Alerts->max_val = atoi(token);
        Alerts->user_console_identifier = atoi(sensor_id);
        createSuccessMessage(message, "OK");
        shared_memory->alert_total++;
        if (msgsnd(message_queue_id, message, sizeof(*message) - sizeof(long), 0) == -1) {
            errorSending(sensor_id);
        }
    } else {
        createErrorMessage(message, "ERROR", token, "alerts list is full");
        if (msgsnd(message_queue_id, message, sizeof(*message) - sizeof(long), 0) == -1) {
            errorSending(sensor_id);
        }
    }
}

void processAlerts(Message* message, char* sensor_id) {
    alert_info* alerts = shared_memory->alerts_array;
    if (show) {
        strcpy(message->message_content, "ID Key MIN MAX");
        if (msgsnd(message_queue_id, message, sizeof(*message) - sizeof(long), 0) == -1) {
            errorSending(sensor_id);
        }
        for (int i = 0; i < shared_memory->alert_total; i++) {
            char temp_content[300];
            sprintf(temp_content, "%s %s %d %d", alerts->sensor_id, alerts->sensor_key, alerts->min_val, alerts->max_val);
            strcpy(message->message_content, temp_content);
            if (msgsnd(message_queue_id, message, sizeof(*message) - sizeof(long), 0) == -1) {
                errorSending(sensor_id);
            }
            alerts++;
        }
        show = false;
    } else {
        char* token = strtok(NULL, "#");
        bool alert_found = false;
        for (int i = 0; i < shared_memory->alert_total; i++) {
            if (strcmp((alerts + i)->sensor_id, token) == 0) {
                *(alerts + i) = *(alerts + (shared_memory->alert_total - 1));
                shared_memory->alert_total--;
                alert_found = true;
                break;
            }
        }
        if (alert_found) {
            strcpy(message->message_content, "OK");
        } else {
            sprintf(message->message_content, "ERRO: %s alert sensor_id doesn't exist", token);
        }
        if (msgsnd(message_queue_id, message, sizeof(*message) - sizeof(long), 0) == -1) {
            errorSending(sensor_id);
        }
    }
}

void processUser(char s[]){
    char *sensor_id, *token, car[] = "#";
    Message message;
    sensor_id = strtok(s, car);
    message.message_type = atol(sensor_id);
    token = strtok(0, car);
    if(!token){
        return;
    }
    char temp_storage[200];
    switch(atoi(token)) {
        case 1:
            processStatistics(&message, sensor_id);
            break;
        case 2:
            resetStatistics(&message, sensor_id);
            break;
        case 3:
            processSensorList(&message, sensor_id);
            break;
        case 4:
            addAlert(&message, sensor_id);
            break;
        case 5:
            processAlerts(&message, sensor_id);
            break;
        case 6:
            show = true;
            processAlerts(&message, sensor_id);
            break;
        default:
            sprintf(temp_storage, "Error: BAD COMMAND: %s\n", s);
            outputToConsole(0, temp_storage);
            break;
    }
}

bool isSensorData(const char* s) {
    return s[strlen(s) - 1] == 'S';
}

void process(int channel[]) {
    char s[123], buffer[200];
    ssize_t size;
    close(channel[1]);  
    sprintf(buffer, "WORKER %d READY", worker_number);
    outputToConsole(0, buffer);
    while (true) {
        size = read(channel[0], s, sizeof(s) - 1); 
        s[size] = '\0'; 
        if (size == 0) {  
            signalHandler(SIGCHLD);
            break;
        } else if (size < 0) { 
            sprintf(buffer, "Error reading on worker %d unnamed_pipe: %s", worker_number, strerror(errno));
            outputToConsole(0, buffer);
            break;
        }
        sem_wait(shared_memory_mutex);
        if (isSensorData(s)) { 
            processSensor(s);
        } else {  
            processUser(s);
        }
        sem_post(shared_memory_mutex);
        sprintf(buffer, "WORKER %d:%s PROCESSING COMPLETE", worker_number, s);
        outputToConsole(0, buffer);
        sem_wait(shared_memory_mutex);
        *(shared_memory->worker_status_array + worker_number) = false;
        sem_post(shared_memory_mutex);
        sem_post(available_workers_semaphore);
    }
}

void checkAndTriggerAlert(alert_info* alert, sensor_info* sensor_stats) {
    if (sensor_stats->last_val < alert->min_val || sensor_stats->last_val > alert->max_val) {
        Message message;
        char temp_storage[10];
        message.message_type = alert->user_console_identifier;
        sprintf(message.message_content, "ALERT %s TRIGGERED: %d", alert->sensor_id, sensor_stats->last_val);
        if (msgsnd(message_queue_id, &message, sizeof(message) - sizeof(long), 0) == -1) {
            sprintf(temp_storage, "%d", alert->user_console_identifier);
            errorSending(temp_storage);
        }
        outputToConsole(0, message.message_content);
    }
}

void markAllAlertsAsTriggered() {
    for (int i = 0; i < shared_memory->sensor_stat_count; ++i) {
        sensor_info* sensor_stats = shared_memory->sensor_stats_array + i;
        sensor_stats->alert_status = true;
    }
}


void alertsWatcher() {
    alert_info* alert;
    sensor_info* sensor_stats;
    Message message;
    char temp_storage[10];
    outputToConsole(0, "Alert Watcher created!");

    while (true) {
        sem_wait(sensor_stat_mutex);
        sem_wait(shared_memory_mutex);

        for (int i = 0; i < shared_memory->alert_total; ++i) {
            alert = shared_memory->alerts_array + i;

            for (int j = 0; j < shared_memory->sensor_stat_count; ++j) {
                sensor_stats = shared_memory->sensor_stats_array + j;

                if (!sensor_stats->alert_status && strcmp(alert->sensor_key, sensor_stats->sensor_key) == 0) {
                    checkAndTriggerAlert(alert, sensor_stats);
                }
            }
        }

        markAllAlertsAsTriggered();

        sem_post(shared_memory_mutex);
    }
}

void *console() {
    char s[125];
    int read_return;
    char error_message[200];
    outputToConsole(0, "Console thread starting!");
    while (true) {
        console_file_descriptor = open("CONSOLE_PIPE", O_RDONLY);
        if (console_file_descriptor < 0) {
            sprintf(error_message, "Cannot open CONSOLE_PIPE for reading: %s", strerror(errno));
            outputToConsole(0, error_message);
            exit(0);
        }
        #ifdef DEBUG
        outputToConsole(2, "CONSOLE_PIPE opened!");
        #endif
        while ((read_return = read(console_file_descriptor, s, sizeof(s))) != 0) {
            if (read_return > 0) {
                #ifdef DEBUG
                sem_wait(output_mutex);
                printf("Received from CONSOLE_PIPE: %s\n", s);
                sem_post(output_mutex);
                #endif
                strcat(s, "#C");
                addToQueue(1, s);
            } else {
                sprintf(error_message, "Error reading from CONSOLE_PIPE: %s", strerror(errno));
                outputToConsole(0, error_message);
            }
        }
        close(console_file_descriptor);
        outputToConsole(2, "CONSOLE_PIPE closed: There are no more pipes opened on the other LAST_NODE");
    }
    return NULL;
}

void* sensor() {
    char s[125];
    outputToConsole(0, "Sensor thread starting!");
    while (1) {
        int sensor_file_descriptor = open(SENSOR_PIPE, O_RDONLY);
        if (sensor_file_descriptor < 0) {
            sprintf(s, "Cannot open SENSOR_PIPE for reading: %s", strerror(errno));
            outputToConsole(0, s);
            exit(0);
        }
        #ifdef DEBUG
        outputToConsole(2, "SENSOR_PIPE opened!");
        #endif
        while (1) {
            ssize_t read_return = read(sensor_file_descriptor, s, sizeof(s));
            if (read_return > 0) {
                #ifdef DEBUG
                sem_wait(output_mutex);
                printf("Received from SENSOR_PIPE: %s\n", s);
                sem_post(output_mutex);
                #endif

                strcat(s, "#S");
                addToQueue(0, s);
            } else if (read_return == -1) {
                sprintf(s, "Error reading from SENSOR_PIPE: %s", strerror(errno));
                outputToConsole(0, s);
                break;
            } else {
                outputToConsole(2, "SENSOR_PIPE closed: There aren't any connections on the other LAST_NODE");
                break;
            }
        }
        close(sensor_file_descriptor);
    }
    return NULL;
}

void *dispatch(void *arg) {
    int i, available_worker_index = 0;
    pipe_array = arg;
    char s[123];
    char temp_storage[200];
    outputToConsole(0, "Dispatch thread starting!");
    while (true) {
        pthread_mutex_lock(&queue_lock);
        if (current_queue_size < 1) {
            #ifdef DEBUG
            outputToConsole(2, "QUEUE is empty! Nothing to pop!");
            #endif
            pthread_cond_wait(&queue_empty_condition, &queue_lock);
        }
        if (popFromQueue(s)) {
            #ifdef DEBUG
            sprintf(temp_storage, "Removed from queue: %s", s);
            outputToConsole(2, temp_storage);
            #endif
            sem_wait(available_workers_semaphore);
            sem_wait(shared_memory_mutex);
            for (i = 0; i < system_config[1]; i++) {
                if (!*(shared_memory->worker_status_array + i)) {
                    available_worker_index = i;
                    *(shared_memory->worker_status_array + i) = true;
                    break;
                }
                #ifdef DEBUG
                sprintf(temp_storage, "Worker %d is occupied!", i);
                outputToConsole(2, temp_storage);
                #endif
            }
            sem_post(shared_memory_mutex);
            sprintf(temp_storage, "DISPATCHER: %s SENT FOR PROCESSING ON WORKER %d", s, available_worker_index);
            outputToConsole(0, temp_storage);
            write(pipe_array[available_worker_index][1], s, strlen(s) + 1);
        }
        pthread_cond_signal(&queue_full_condition);
        pthread_mutex_unlock(&queue_lock);
    }
    return NULL;
}




