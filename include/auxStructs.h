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

#define MESSAGE_SIZE 4096

// Linked List of Commands

typedef struct llCommand{
    char* command;
    char** args;
    struct llCommand *next;
} *LlCommand;

LlCommand newLLC(char* c,char** args);

void freeLLC(LlCommand list);

void appendLLC(LlCommand list, char* c);

char* getCommand(LlCommand, int n);

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

LinkedListProcess parseProcess(char *str, int pid_client);

void freeProcess(LinkedListProcess process);

void appendsProcess(LinkedListProcess l, LinkedListProcess p);

LinkedListProcess removeProcessesHead(LinkedListProcess *list);

//Dá print da info do processo, para o ficheiro fornecido
//Admite 'process' não nulo
void printProcessInfo(int fildes, LinkedListProcess process);

LinkedListProcess removeProcessByChildPid(LinkedListProcess *list, pid_t pid);

/** Outras **/

ssize_t readln(int fd, char* line, size_t size);

#endif
