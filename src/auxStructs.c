#include "../include/auxStructs.h"

/** Linked List of Commands **/

LlCommand newLLC(char* c,char** args){
    LlCommand l = malloc(sizeof(struct llCommand));
    l->command = c;
    l->args=args;
    l->next   = NULL;
    return l;
}

void freeLLC(LlCommand list){
    LlCommand tmp = list;

    while(tmp != NULL){
        tmp = tmp->next;
        free(list->command);
        for(int i=0;list->args[i]!=NULL;i++)free(list->args[i]);
        free(list->args);
        free(list);
        list = tmp;
    }
}

void appendsLLC(LlCommand llc, char* c, char** args){
    LlCommand l = newLLC(c,args);
    while (llc->next != NULL)
        llc = llc->next;
    llc->next = l;
}

char* getCommand(LlCommand llc,int n ){
    //Percorre a linked list, à procura do command
    LlCommand tmp;
    int i = 0;
    for(tmp = llc; tmp != NULL && i<n; tmp = tmp->next) i++;

    //Se tmp != NULL então encontrou o comando
    if(tmp != NULL) return tmp->command;

    return NULL;
}

/** Lista ligada com a informacao dos processos **/

LinkedListProcess parseProcess(char *str, int pid_client){
    LinkedListProcess p = (LinkedListProcess) malloc(sizeof(struct linkedListProcess));
    p->pid_child    = -1;
    p->task_number  = 0;
    p->commandsCount = 0;
    p->next         = NULL;



    if (str[0]=='s'){

    }else {
        //ignora "execute"
        strsep(&str," ");

        //ignora time
        strsep(&str," ");

        if (str[1]=='u'){
        
            //ignora "-u"
            strsep(&str," ");

            //Gets pid from client
            p->pid_client = pid_client;

            //gets output files
            p->output_file  = NULL;
            p->priority  = 0;

            //gets commands
            int maxCommands = 1;
            char *command;
            char **args=malloc(sizeof(char*)*256);
            int z=0,j=0,i=0;
		    // ver quantidade de characteres ate o proximo espaco
		    while(str[j] != ' '){
			    j++;
		    }
		    j++;
            // colocar o nome do commando no arg[0]
		    args[0] = malloc(sizeof(char)*(j-z));
		    strncpy(args[0],&str[z],j-z);
		    z=j;
		    // funcao para colocar os argumentos do commando nos respectivos args
		    while(strsep(&str," ")){
                printf("AAAAA");
			    if (str[j]==' ' || str[j]=='\0') {
			    	args[i] = malloc(sizeof(char)*(j-z));
			    	strncpy(args[i],&str[z],j-z);
			    	z = j + 1;
			    	printf("arg: %s ", args[i]);
			    	i++;
		    	if (str[j]=='\0') {
		    		args[i] = NULL;
				    }
			        j++;
		        }
                LlCommand llc = newLLC(args[0],args);
                return p;
            }
        }else if (str[1]=='p'){
            //ignora "-p"
            strsep(&str," ");

            //Gets pid from client
            p->pid_client = pid_client;

            //gets output files
            p->output_file  = NULL;
            p->priority  = 0;

            //gets commands
            int maxCommands = 1;
            char *command;

            while((command = strsep(&str,"|")) != NULL){
                //Caso de não haver espaço suficiente para os comandos
                if(p->commandsCount == maxCommands){
                    maxCommands *= 2;
                    p->commands = realloc(p->commands, maxCommands * sizeof(char *));
                }

                //Coloca o comando na estrutura
                LlCommand tmp;
                
                p->commandsCount++;
            }

            return p;
        
        }
    }
    return 0;
}

void freeProcess(LinkedListProcess process){
    if(process != NULL){
        free(process->output_file);
        freeLLC(process->commands);
        free(process);
    }
}

//Appends process to the linked list
void appendsProcess(LinkedListProcess l, LinkedListProcess p){

    LinkedListProcess next = l->next;

    while(next != NULL && l->priority >= p->priority){
        l = l->next;
        next = l->next;
    }

    p->next = next;
    l->next = p;
}

//Dá print da info do processo, para o ficheiro fornecido
//Admite 'process' não nulo
void printProcessInfo(int fildes, LinkedListProcess process){
    char buffer[1024];

    sprintf(buffer, "task #%d: process %s\0", process->task_number, process->output_file);
    for(int i = 0; i < process->commandsCount ; i++) {
        strcat(buffer, " ");
        strcat(buffer,process->commands->command);
    }

    strcat(buffer,"\n");
    if(write(fildes, buffer, strlen(buffer))==-1){
        perror("write");
    }
}

LinkedListProcess removeProcessByChildPid(LinkedListProcess *list, pid_t pid){
    if(list == NULL || *list == NULL) return NULL;

    LinkedListProcess prev = NULL, tmp;

    for(tmp = *list; tmp != NULL && tmp->pid_child != pid ; prev = tmp, tmp = tmp->next);

    if(tmp != NULL){
        //Se prev for NULL, entao o pid foi encontrado no primeiro elemento da lista
        if(prev == NULL){
            *list = tmp->next;
        }
        else{
            prev->next = tmp->next;
        }

        tmp->next = NULL;
        return tmp;
    }

    return NULL;
}

LinkedListProcess removeProcessesHead(LinkedListProcess *list){
    LinkedListProcess tmp = *list;
    *list = tmp->next;
    tmp->next = NULL;
    return tmp;
}

/** Outros **/

ssize_t readln(int fd, char* line, size_t size) {
    ssize_t bytes_read = read(fd, line, size);
    if(!bytes_read) return 0;

    size_t line_length = strcspn(line, "\n") + 1;
    if(bytes_read < line_length) line_length = bytes_read;
    line[line_length] = 0;

    lseek(fd, line_length - bytes_read, SEEK_CUR);
    return line_length;
}
