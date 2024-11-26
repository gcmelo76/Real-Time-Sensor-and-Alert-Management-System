/*

Joao Pedro Morais Gaspar          nº2021221276
Guilherme Craveiro de Melo     nº 2021217138

*/
#ifndef OTHERFUNCTIONS_H
#define OTHERFUNCTIONS_H

#include "mainFunctions.h"
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
void outputToConsole(short int function,char s[]);
void signalHandler(int signo);

INTERNAL_LIST* createNewNode(short int priority_level, char message_content[]);
INTERNAL_LIST* findInsertionPoint();
void insertNode(INTERNAL_LIST *insertion_point, INTERNAL_LIST *new_node);
void addToQueue(short int priority_level,char message_content[]);


int popFromQueue(char dest_string[]);

bool findSensorById(char* sensor_id);

void addNewSensor(char* sensor_id);

bool findSensorStatsByKey(char* sensor_key);

void updateSensorStats(char* sensor_key, int value);

void addNewSensorStats(char* sensor_key, int value);

sensor_info* getSensorStatsByKey(char* sensor_key);

void processSensor(char s[]);



void errorSending(char sensor_id[]);

void processStatistics(Message* message, char* sensor_id);

void resetStatistics(Message* message, char* sensor_id);

void processSensorList(Message* message, char* sensor_id);

void addAlert(Message* message, char* sensor_id);

void processAlerts(Message* message, char* sensor_id);


void processUser(char s[]);

bool isSensorData(const char* s);

void process(int channel[]);

void checkAndTriggerAlert(alert_info* alert, sensor_info* sensor_stats);

void markAllAlertsAsTriggered();

void alertsWatcher();

void *console();

void *sensor();

void *dispatch(void *arg);

#endif
