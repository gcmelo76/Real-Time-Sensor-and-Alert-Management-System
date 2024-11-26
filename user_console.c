/*

Joao Pedro Morais Gaspar          nº2021221276
Guilherme Craveiro de Melo     nº 2021217138

*/


#include "commonFunctions.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <stdbool.h>

#define DEBUG_MODE

typedef struct {
    long type;
    char data[88];
} Message;


int pipe_descriptor,msg_queue_id;
pthread_t msg_receiver;
long msg_type;

void* receive_msgs(void* arg) {
    Message received_msg;
    while (true) {
        if (msgrcv(msg_queue_id, &received_msg, sizeof(received_msg) - sizeof(long), msg_type, 0) == -1) {
            perror("Erro ao receber mensagem");
            exit(EXIT_FAILURE);
        }
        printf("%s\n", received_msg.data);
    }
    return NULL;
}

void init_receiver_thread() {
    int error_code;
    error_code = pthread_create(&msg_receiver, NULL, &receive_msgs, NULL);
    if (error_code != 0) {
        printf("%s\n", strerror(error_code));
    }
}

void signal_handler(int signal_num) {
    if (signal_num == SIGPIPE) {
        printf("Erro: Pipe quebrado\n");
    }
    close(pipe_descriptor);
    close(msg_queue_id);
    pthread_cancel(msg_receiver);
    exit(EXIT_SUCCESS);
}

void handle_stats(const char *argv[], char *output_cmd, bool *cmd_accepted) {
    sprintf(output_cmd, "%s#1", argv[1]);
    *cmd_accepted = true;
}

void handle_reset(const char *argv[], char *output_cmd, bool *cmd_accepted) {
    sprintf(output_cmd, "%s#2", argv[1]);
    *cmd_accepted = true;
}

void handle_sensors(const char *argv[], char *output_cmd, bool *cmd_accepted) {
    sprintf(output_cmd, "%s#3", argv[1]);
    *cmd_accepted = true;
}

void handle_add_alert(const char *argv[], char *output_cmd, bool *cmd_accepted, char *input_cmd) {
    char syntax_add_alert[] = "Verifique a sintaxe: add_alert [id] [chave] [min] [max]\n";

    strtok(input_cmd, " ");
    char *s1 = strtok(0, " ");
    char *s2 = strtok(0, " ");
    char *s3 = strtok(0, " ");
    char *s4 = strtok(0, "\n");

    if (s1 == 0 || !verify_alphanumeric(0, s1) || s2 == 0 || !verify_alphanumeric(1, s2) || s3 == 0 || !determine_integer(0, s3) || s4 == 0 || !determine_integer(0, s4)) {
        printf("O id de um alerta é um código alfanumérico com um tamanho mínimo de 3 caracteres e máximo de 32 caracteres!\n");
        printf("A chave é uma string com tamanho entre 3 e 32 caracteres (inclusive) formada por uma combinação de dígitos, caracteres alfabéticos e underscore (_).\n");
        printf("[min] e [max] têm de ser inteiros.\n");
        printf("%s", syntax_add_alert);
    } else {
        int min = atoi(s3);
        int max = atoi(s4);
        if (min > max && max < min) {
            printf("[min] não pode ser maior do que [max], nem vice-versa!\n");
            printf("%s", syntax_add_alert);
        } else {
            sprintf(output_cmd, "%s#4#%s#%s#%d#%d", argv[1], s1, s2, min, max);
            *cmd_accepted = true;
        }
    }
}

void handle_remove_alert(const char *argv[], char *output_cmd, bool *cmd_accepted, char *input_cmd) {
    char syntax_remove_alert[] = "Verifique a sintaxe: remove_alert [id]\n";

    strtok(input_cmd, " ");
    char *id = strtok(0, "\n");

    if (id == 0 || !verify_alphanumeric(0, id)) {
        printf("O id de um alerta é um código alfanumérico com um tamanho mínimo de 3 caracteres e máximo de 32 caracteres!\n");
        printf("%s", syntax_remove_alert);
    } else {
        sprintf(output_cmd, "%s#5#%s", argv[1], id);
        *cmd_accepted = true;
    }
}

void handle_list_alerts(const char *argv[], char *output_cmd, bool *cmd_accepted) {
    sprintf(output_cmd, "%s#6", argv[1]);
    *cmd_accepted = true;
}

void handle_exit() {
    signal_handler(SIGTERM);
}

void handle_bad_command(const char menu) {
    printf("Bad Command\n%s", menu);
}

void handle_user_input(char *input_cmd, const char *argv[], char *output_cmd, bool *cmd_accepted, const char *menu) {
    *cmd_accepted = false;

    if (strncmp("stats", input_cmd, 5) == 0) {
        handle_stats(argv, output_cmd, cmd_accepted);
    } else if (strncmp("reset", input_cmd, 5) == 0) {
        handle_reset(argv, output_cmd, cmd_accepted);
    } else if (strncmp("sensors", input_cmd, 7) == 0) {
        handle_sensors(argv, output_cmd, cmd_accepted);
    } else if (strncmp("add_alert", input_cmd, 9) == 0) {
        handle_add_alert(argv, output_cmd, cmd_accepted, input_cmd);
    } else if (strncmp("remove_alert", input_cmd, 12) == 0) {
        handle_remove_alert(argv, output_cmd, cmd_accepted, input_cmd);
    } else if (strncmp("list_alerts", input_cmd, 11) == 0) {
        handle_list_alerts(argv, output_cmd, cmd_accepted);
    } else if (strncmp("exit", input_cmd, 4) == 0) {
        handle_exit();
    } else {
        handle_bad_command(menu);
    }
}
                 
void send_output_to_pipe(int pipe_descriptor, const char* output_cmd, const char* argv[]) {
    int write_result = write(pipe_descriptor, output_cmd, strlen(output_cmd) + 1); 
    if (write_result == -1) {
        printf("user_console %s: erro ao escrever no CONSOLE_PIPE\n", argv[1]);
    }
}

void execute_user_commands(const char* argv[]) {
    const char menu[] = "1) exit\n2) stats\n3) reset\n4) sensors \n5) add_alert [id] [chave] [min] [max]\n6) remove_alert [id] \n7) list_alerts \n";

    printf("%s", menu);

    char input_cmd[100];
    char output_cmd[123];
    bool cmd_accepted = false;

    while (true) {
        fgets(input_cmd, 100, stdin);
        handle_user_input(input_cmd, argv, output_cmd, &cmd_accepted, menu);

        if (cmd_accepted) {
            cmd_accepted = false;
            send_output_to_pipe(pipe_descriptor, output_cmd, argv);
        }
    }
}


int main(int argc_num, char const *arg_strings[]) {

    if (argc_num < 2) {
        printf("Sintaxe do comando a executar na linha de comando:\n$ user_console {identificador da consola}\n");
        exit(0);
    }
    if (!determine_integer(-1, arg_strings[1])) {
        printf("O parâmetro identificador da consola tem de ser um valor inteiro superior a 0\n");
        exit(0);
    }

    if ((pipe_descriptor = open("CONSOLE_PIPE", O_WRONLY)) < 0) {
        perror("Não é possível abrir CONSOLE_PIPE para escrita");
        exit(0);
    }

    key_t ipc_key = ftok("user_console.c", 'B');
    if (ipc_key == -1) {
        perror("Erro na geração da chave ftok");
        exit(0);
    }

    msg_queue_id = msgget(ipc_key, IPC_CREAT | 0700);
    if (msg_queue_id == -1) {
        perror("Não é possível obter a fila de mensagens");
        close(pipe_descriptor);
        exit(0);
    }
    
    struct sigaction sigact;
    sigact.sa_handler = signal_handler;
    sigact.sa_flags = 0;
    sigfillset(&sigact.sa_mask);
    sigaction(SIGPIPE, &sigact, NULL);
    sigaction(SIGINT, &sigact, NULL);

    msg_type = atol(arg_strings[1]);
    init_receiver_thread();

    execute_user_commands(arg_strings);

    return 0;
}
