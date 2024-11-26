/*

Joao Pedro Morais Gaspar          nº2021221276
Guilherme Craveiro de Melo     nº 2021217138

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include "commonFunctions.h"
#include <time.h>

unsigned int message_count = 0;
int pipe_descriptor;
int write_result;
char sensor_details[67];
char msg_buffer[77];

void handle_pipe_interruption(int signo) {
    if (signo == SIGPIPE) printf("Erro: Pipe Quebrado\n");
    close(pipe_descriptor);
    exit(0);
}

void handle_stop_signal() {
    printf("Total de mensagens enviadas: %d\n", message_count);
}

int generate_random_number(int lower_limit, int upper_limit) {
    return (rand() % (upper_limit - lower_limit + 1)) + lower_limit;
}

void build_sensor_info(char *sensor_data, const char *parameters[]) {
    sprintf(sensor_data, "%s#%s#", parameters[1], parameters[3]);
}

void build_message_string(char *message, const char *sensor_data, int sensor_value) {
    sprintf(message, "%s%d", sensor_data, sensor_value);
}

void register_signal_handlers() {
    struct sigaction act1, act2;
    act1.sa_handler = handle_stop_signal;
    act2.sa_handler = handle_pipe_interruption;
    act1.sa_flags = act2.sa_flags = 0;
    sigaddset(&act1.sa_mask, SIGTSTP);
    sigfillset(&act2.sa_mask);
    sigaction(SIGPIPE, &act2, NULL);
    sigaction(SIGINT, &act2, NULL);
    sigaction(SIGTSTP, &act1, NULL);
}

int main(int arg_count, char *args[]) {
    if (arg_count < 6 || !determine_integer(1, args[2]) || !determine_integer(0, args[4]) || !determine_integer(0, args[5])) {
        printf("Command syntax: \n$ sensor {sensor id} {send interval in seconds (>=0)} {key} {minimum integer value to be sent} {maximum integer value to be sent}\n");
        exit(0);
    }

    if (!verify_alphanumeric(0, args[1])) {
        printf("The sensor identifier must be an alphanumeric code with a minimum length of 3 characters and a maximum length of 32 characters!\n");
        exit(0);
    }
    if (!verify_alphanumeric(1, args[3])) {
        printf("The key is a string with a size between 3 and 32 characters (inclusive) formed by a combination of digits, alphabetic characters and underscore (_)!\n");
        exit(0);
    }

    int value, minimum, maximum, interval;
    minimum = atoi(args[4]);
    maximum = atoi(args[5]);
    if (minimum > maximum && maximum < minimum) {
        printf("[min] cannot be greater than [max], nor vice versa!\n");
        exit(0);
    }
    interval = atoi(args[2]);
    srand(time(NULL));

    register_signal_handlers();

    if ((pipe_descriptor = open("SENSOR_PIPE", O_WRONLY)) < 0) {
        perror("Could not open SENSOR_PIPE for writing");
        exit(0);
    }
    build_sensor_info(sensor_details, args);
    while (1) {
        value = generate_random_number(minimum, maximum);
        build_message_string(msg_buffer, sensor_details, value);
        write_result = write(pipe_descriptor, &msg_buffer, strlen(msg_buffer) + 1);
        if (write_result == -1) {
            printf("Erro ao escrever no SENSOR_PIPE\n");
            message_count--;
        }
        message_count++;
        sleep(interval);
    }
    return 0;
}

