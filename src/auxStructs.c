#include "../include/auxStructs.h"

// Linked List of Commands
LlCommand newLLC(){
    LlCommand l = malloc(sizeof(struct llCommand));
    l->args= NULL;
    l->next   = NULL;
    return l;
}

// Appends a command to the linked list
void freeLLC(LlCommand list){
    LlCommand tmp = list;

    //Percorre a linked list, libertando a memoria alocada
    while(tmp != NULL){
        tmp = tmp->next;
        for(int i=0;list->args[i]!=NULL;i++)free(list->args[i]);
        free(list->args);
        free(list);
        list = tmp;
    }
}

// Gets the arguments of the command
char** getArgs(LlCommand llc,int n ){
    LlCommand tmp;
    int i = 0;

    //Percorre a linked list até encontrar o comando n
    for(tmp = llc; tmp != NULL && i<n; tmp = tmp->next) i++;

    //Se encontrar o comando n, retorna os argumentos
    if(tmp != NULL) return tmp->args;

    //Se não encontrar, retorna NULL
    return NULL;
}

// Linked List of Processes
LinkedListProcess parseProcess(char *str, int pid_client,int outputsize,int task_number){
    LinkedListProcess p = (LinkedListProcess) malloc(sizeof(struct linkedListProcess));
    p->pid_child    = -1;
    p->task_number  = task_number;
    p->commandsCount = 0;
    p->next         = NULL;
    LlCommand llc = newLLC();
    p->commands = llc;

    //Se o comando for "status", então o processo é um comando status
    if (str[0]=='s'){
        p->commands->args=malloc(sizeof(char*)*256);
        p->output_file  = malloc(sizeof(char)*outputsize);
        p->commands->args[0] = malloc(sizeof(char)*5);
        strcpy(p->commands->args[0],"status");
        p->pid_client = pid_client;
        p->priority = -1;
        printf("Status made\n");
        return p;
    
    //Se não for "status", então é um comando execute
    }else{

            //ignora "execute"
            strsep(&str," ");
            //ignora -u ou -p
            strsep(&str," ");
            //ignora tempo
            strsep(&str," ");
            //Gets pid from client
            p->pid_client = pid_client;

            //Aloca espaço para o nome do ficheiro de output
            p->output_file  = malloc(sizeof(char)*outputsize);
            p->priority  = 0;
            p->commands->args=malloc(sizeof(char*)*256);

            int z=0,j=1,i=0;
            
            //Percorre a string, separando os argumentos por espaços
            LlCommand tmp = p->commands;
		    while(j<strlen(str)+1){
                switch(str[j]){
                    case '|':
                        p->commandsCount++;
                        tmp->next = newLLC();
                        tmp=tmp->next;
                        tmp->args = malloc(sizeof(char*)*256);
                        i=0;
                        j=j+2;
                        z=j;
                        j++;
                        break;
                    case ' ':
                        tmp->args[i] = malloc(sizeof(char)*(j-z));
                        strncpy(tmp->args[i],&str[z],j-z);
                        j++;
                        z = j;
                        i++;
                        break;
                    case '\0':
                        tmp->args[i] = malloc(sizeof(char)*(j-z));
                        strncpy(tmp->args[i],&str[z],j-z);
                        j++;
                        z = j;
                        i++;
                        tmp->args[i] = NULL;
                        j++;
                        break;
                    default:
                        j++;
                        break;
                }
                
            }
        }
    //Retorna o processo
    return p;
    }

//Frees the process
void freeProcess(LinkedListProcess process){

    //Liberta a memoria alocada para o nome do ficheiro de output
    if(process != NULL){
        free(process->output_file);
        freeLLC(process->commands);
        if (process->next != NULL) freeProcess(process->next);
        free(process);
    }
}

// Appends a process to the linked list
void appendsProcess(LinkedListProcess l, LinkedListProcess p){

    LinkedListProcess next = l->next;

    //Percorre a linked list até encontrar um processo com prioridade menor que o processo a adicionar
    while(next != NULL && l->priority >= p->priority){
        l = l->next;
        next = l->next;
    }

    //Adiciona o processo à linked list
    p->next = next;
    l->next = p;
}

// Prints the process info to the file descriptor
void printProcessInfo(int fildes, LinkedListProcess process){
    char buffer[1024];

    //Cria a string com a informação do processo
    sprintf(buffer, "task #%d: process %s", process->task_number, process->output_file);
    LlCommand tmp = newLLC();

    //Adiciona os comandos à string
    for(tmp = process->commands;tmp!=NULL;tmp=tmp->next){
    for(int i = 0; (tmp->args[i])!= NULL ; i++) {
        strcat(buffer, " ");
        strcat(buffer,tmp->args[i]);
        }
        if (tmp->next!=NULL)strcat(buffer," |");
    }
    strcat(buffer,"\n");

    //Escreve a string no ficheiro
    if(write(fildes, buffer, strlen(buffer))==-1){
        perror("write");
    }
}

// Removes a process from the linked list by task number
LinkedListProcess removeProcessByTaskNumber(LinkedListProcess list, int task_number){

    //Se a lista for NULL, retorna NULL
    if(list == NULL) return NULL;

    LinkedListProcess prev = NULL, tmp;

    //Percorre a linked list até encontrar o processo com o task_number
    for(tmp = list; tmp != NULL && tmp->task_number != task_number ; tmp = tmp->next)if (tmp->next!=NULL) prev = tmp;

    //Se encontrar o processo, remove-o da linked list
    if(tmp != NULL){
        
        //Se o processo for o primeiro da linked list
        if(prev == NULL){
            list = tmp->next;
        }

        //Se não for o primeiro
        else{
            prev->next = tmp->next;
            tmp->next = NULL;
        }

        //Retorna o processo removido
        return tmp;
    }

    //Se não encontrar o processo, retorna NULL
    return NULL;
}

// Removes the head of the linked list
LinkedListProcess removeProcessesHead(LinkedListProcess list){

    //Se a lista for NULL, retorna NULL
    LinkedListProcess tmp = list;

    //Se a lista não for NULL, remove o primeiro elemento
    list = tmp->next;
    tmp->next = NULL;
    return tmp;
}