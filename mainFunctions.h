/*

Joao Pedro Morais Gaspar          nº2021221276
Guilherme Craveiro de Melo     nº 2021217138

*/
#ifndef MAINFUNCTIONS_H
#define MAINFUNCTIONS_H

#include "otherFunctions.h"
#include "system_manager.h"
#include "commonFunctions.h"

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

void setupSignals() ;

void setupLogFile() ;

void verifyArguments(int argc, char const *argv[]) ;

void setupConfigFile(char const *argv[]);

void printErrorAndExit(const char *message);

void createSharedMemory() ;

void attachSharedMemory() ;

void setupKeyAndMessageQueue() ;


void setupSemaphores() ;

void setupSigaction();
void setupInternalQueue() ;

void setupChildProcess(int i);
void setupAlertWatcherProcess();
void handleWorkerCreationError();
void handleAlertWatcherCreationError();
void spawnProcesses();

int createNamedPipe(const char* pipeName);
void handlePipeCreationError(const char* pipeName);
void setupPipes();

int createThread(pthread_t* thread, void* (*start_routine)(void*), void* arg);
void handleThreadCreationError(int error_code);
void createThreads();

void closePipesAndKillProcesses() ;

int unlinkNamedPipe(const char* pipeName);
int closeFileDescriptor(int fileDescriptor);
int detachSharedMemory(void* sharedMemory);
void handleUnlinkError(const char* pipeName);
void handleCloseError(int fileDescriptor);
void handleDetachError(void* sharedMemory);



void unlinkAndCloseResources() ;

void waitForAllProcessesToFinish() ;

void unlinkSemaphores();

void removeIPCResources();

void cleanUp() ;

#endif
