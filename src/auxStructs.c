#include "../include/auxStructs.h"

/** Linked List of Commands **/

LlCommand newLLC(){
    LlCommand l = malloc(sizeof(struct llCommand));
    l->command = NULL;
    l->args= NULL;
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

LinkedListProcess parseProcess(char *str, int pid_client,int outputsize){
    LinkedListProcess p = (LinkedListProcess) malloc(sizeof(struct linkedListProcess));
    p->pid_child    = -1;
    p->task_number  = 0;
    p->commandsCount = 0;
    p->next         = NULL;
    LlCommand llc = newLLC();
    p->commands = llc;

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
            p->output_file  = malloc(sizeof(char)*outputsize);
            p->priority  = 0;
            
            p->commands->args=malloc(sizeof(char*)*256);

            int z=0,j=1,i=1;
		    // ver quantidade de characteres ate o proximo espaco
		    while(str[j] != ' '){
			    j++;
		    }
		    j++;
            // colocar o nome do commando no arg[0]
                                write(1,"hello\n",6);

		    p->commands->args[0] = malloc(sizeof(char)*(j-z));

		    strncpy(p->commands->args[0],&str[z],j-z-1);
            p->commandsCount++;
            printf("arg: %s ", p->commands->args[i]);
		    z=j;
		    // funcao para colocar os argumentos do commando nos respectivos args
		    while(j<strlen(str)+1){
			    if (str[j]==' ') {
			    	p->commands->args[i] = malloc(sizeof(char)*(j-z));
			    	strncpy(p->commands->args[i],&str[z],j-z);
			    	z = j + 1;
			    	printf("arg: %s ", p->commands->args[i]);
			    	i++;
		        
                } 
                else if (str[j]=='\0'){
			    	p->commands->args[i] = malloc(sizeof(char)*(j-z));
			    	strncpy(p->commands->args[i],&str[z],j-z);
			    	z = j + 1;
			    	printf("arg: %s ",p->commands->args[i]);
			    	i++;
		    	
		    		p->commands->args[i] = NULL;
				    
			        j++;
            
                }
                j++;
            }
                if (p->commands->args[i]==NULL)printf("arg: NULL\n");
                for(int g =0;(p->commands->args[g])!=NULL;g++){
                printf("args v2 : %s\n",p->commands->args[g]);
            }
        }else if (str[1]=='p'){
         
            //ignora "-u"
            strsep(&str," ");

            //Gets pid from client
            p->pid_client = pid_client;

            //gets output files
            p->output_file  = malloc(sizeof(char)*outputsize);
            p->priority  = 0;
            
            p->commands->args=malloc(sizeof(char*)*256);

            int z=0,j=1,i=1;
		    // ver quantidade de characteres ate o proximo espaco
		    while(str[j] != ' '){
			    j++;
		    }
		    j++;
            // colocar o nome do commando no arg[0]

		    p->commands->args[0] = malloc(sizeof(char)*(j-z));

		    strncpy(p->commands->args[0],&str[z],j-z-1);
            p->commandsCount++;
            printf("arg: %s ", p->commands->args[i]);
		    z=j;
		    // funcao para colocar os argumentos do commando nos respectivos args
            LlCommand tmp = p->commands;
		    while(j<strlen(str)+1){
                if(str[j]== '|'){
                tmp->next = newLLC();
                tmp=tmp->next;
                i=0;
                j++;
                }
			    if (str[j]==' ') {
			    	tmp->args[i] = malloc(sizeof(char)*(j-z));
			    	strncpy(tmp->args[i],&str[z],j-z);
			    	z = j + 1;
			    	printf("arg: %s ", tmp->args[i]);
			    	i++;
		        
                } 
                else if (str[j]=='\0'){
			    	tmp->args[i] = malloc(sizeof(char)*(j-z));
			    	strncpy(tmp->args[i],&str[z],j-z);
			    	z = j + 1;
			    	printf("arg: %s ",tmp->args[i]);
			    	i++;
		    	
		    		tmp->args[i] = NULL;
				    
			        j++;
            
                }
                j++;
                
            }
                if (p->commands->args[i]==NULL)printf("arg: NULL\n");
                for(int g =0;(p->commands->args[g])!=NULL;g++){
                printf("args v2 : %s\n",p->commands->args[g]);
            
            }
        }
    }
    return p;
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

    sprintf(buffer, "task #%d: process %s", process->task_number, process->output_file);
    for(int i = 0; (process->commands->args[i])!= NULL ; i++) {
        strcat(buffer, " ");
        strcat(buffer,process->commands->args[i]);
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
