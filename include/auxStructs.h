#ifndef AUXSTRUCTS_H
#define AUXSTRUCTS_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "signal.h"
#include "sys/wait.h"
#include <sys/queue.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>
#include <sys/time.h>

#define MESSAGE_SIZE 4096

// Linked List of Commands

typedef struct llCommand{
    char** args;
    struct llCommand *next;
} *LlCommand;

LlCommand newLLC();

void freeLLC(LlCommand list);

void appendLLC(LlCommand list, char* c);

char** getArgs(LlCommand llc,int n);

/** Lista ligada com a informacao dos processos **/

typedef struct linkedListProcess{
    pid_t pid_client;
    pid_t pid_child; //Pid do filho que vai executar a task
    int task_number;
    char *output_file;
    int priority;
    int commandsCount;
    LlCommand commands;
    struct linkedListProcess *next;
} *LinkedListProcess;

//LinkedListProcess newProcess(pid_t pid, int task_number, char *input_file, char *output_file, int commandsCount, char **commands);

LinkedListProcess parseProcess(char *str, int pid_client, int outputsize, int task_number);

void freeProcess(LinkedListProcess process);

void appendsProcess(LinkedListProcess l, LinkedListProcess p);

LinkedListProcess removeProcessesHead(LinkedListProcess list);

//Dá print da info do processo, para o ficheiro fornecido
//Admite 'process' não nulo
void printProcessInfo(int fildes, LinkedListProcess process);

LinkedListProcess removeProcessByTaskNumber(LinkedListProcess list, int task_number);

#endif
