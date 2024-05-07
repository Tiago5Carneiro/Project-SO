#include <unistd.h> 	/* chamadas ao sistema: defs e decls essenciais */
#include <fcntl.h>  	/* O_RDONLY, O_WRONLY, O_CREAT, O_APPEND, O_RDWR , O_TRUNC, O_EXCL, O_* */
#include <stdlib.h> 	/* Malloc */
#include <stdio.h>  	/* printf */
#include <signal.h> 	/* signals (SIGTERM, SIGALARM)*/
#include <sys/stat.h> 	/* mkfifo */
#include <sys/wait.h> 	/* wait */
#include <sys/types.h>
#include <string.h>
#include "../include/auxStructs.h"
#include <semaphore.h>


#define BUFFER_SIZE 1024

static volatile int is_open = 1;
static volatile int current_processing = 0;
static volatile int max_process = 0;

char* output_path;
char buff[BUFFER_SIZE];

// ./orchestrator {output_folder} {parallel-tasks} {sched-policy}

// Funcao para dar flip a uma string
void flip(char* string){
    int i,j;
    
    char aux;

    for (i = 0, j = strlen(string) - 1 ; i < j; i++,j--)
    {
        aux = string[i];
        string[i] = string[j];
        string[j] = aux;
    }
}

// Definir itoa para poder usar quando sao criados os fifos de cada cliente
void itoa(int n, char* string){
    int i, sign;

    // Verificar o sinal 
    if ((sign = n) < 0)n = -n;

    i = 0;

    // Gerar os digitos na ordem inversa
    do
    {                  
    	// Prox digito       
        string[i++] = n % 10 + '0';
    } while ((n /= 10) > 0); // Eliminar digito
    

    if (sign < 0)
        string[i++] = '-';
    string[i] = '\0';

    flip(string);
}

// Funcao para lidar com o fecho do servidor
void handle_signalterm(){
	printf("Closing server...\n");

	is_open=0;

	unlink("./tmp/server_fifo");
}

// Funcao para lidar com Ctrl + C
void handle_signalint(){

	printf("\n\nUNEXPECTED TERMINATION \n\n");

	// Eliminar fifos
	
	// Colocar o handler para o sinal int (Ctrl + C) de volta para a funcao default do kernel (SIG_DFL)
	signal(SIGINT,SIG_DFL);

	// Mandar um singal sig int para si novamente 
	kill(getpid(),SIGINT);
}

int process(LinkedListProcess p){
	int pid_child;
	if ( (pid_child = fork()==0)){
		
		write(1,"HELLO\n",6);
        //Criacao do ficheiro no qual ficará o resultado final de aplicar os comandos
        int output_fp = open(p->output_file, O_CREAT | O_WRONLY | O_TRUNC , 0644);
        if(output_fp == -1) { 
			perror("Open Output file"); _exit(1); 
			}

		printProcessInfo(1,p);
        int commandsCount = p->commandsCount;
        int breadth;
        int pipes[commandsCount - 1][2]; //Tendo N comandos, temos N - 1 pipes
		
	//Gera os diversos pipes que os processos vão usar para comunicar entre eles
        for(breadth = 0; breadth < commandsCount ; breadth++){

            // É o primeiro comando
            if(breadth == 0){

                //Situação de apenas ter um comando
                if(commandsCount == 0){

                    if(fork() == 0){

                        //Redirecionamento do stdout para o ficheiro de output fornecido
                        dup2(output_fp, STDOUT_FILENO);
                        close(output_fp);

                        //Execucao do comando
                        execvp(getCommand(p->commands,breadth),getArgs(p->commands,breadth));
                    }
                }
                //É o primeiro mas existem mais comandos, logo é preciso pelo menos um pipe
                else{

                    //Cria o pipe
                    if(pipe(pipes[0]) == -1) _exit(1);

                    if(fork() == 0){
                        //Fecha o extremo de leitura do pipe
                        close(pipes[0][0]);

                        //Redirecionamento do stdin para o ficheiro de input fornecido

                        //Redirecionamento do stdout para o extremo de escrita do pipe
                        dup2(pipes[0][1], STDOUT_FILENO);

                        //Execucao do comando
                        execvp(getCommand(p->commands,breadth),getArgs(p->commands,breadth));
                    }
                    else{
                        //Fecha o extremo de escrita do pipe
                        close(pipes[0][1]);
                    }
                }
            }
            //É o último comando (Apenas quando tem mais do que um comando)
            else if(breadth == commandsCount){

                if(fork() == 0){
                    //Redirecionamento do stdin para o extremo de leitura do pipe
                    dup2(pipes[breadth - 1][0], STDIN_FILENO);
                    close(pipes[breadth - 1][0]);

                    //Redirecionamento do stdout para o ficheiro de output fornecido
                    dup2(output_fp, STDOUT_FILENO);
                    close(output_fp);

                    //Execucao do comando
                    execvp(getCommand(p->commands,breadth),getArgs(p->commands,breadth));
                }
                else{
                    //Fecha o extremo de escrita do pipe
                    close(pipes[breadth - 1][0]);
                    close(output_fp);
                }
            }
            //É comando intermédio
            else{
                //Cria o pipe comando
                if(pipe(pipes[breadth]) == -1) _exit(1);

                if(fork() == 0){
                    //Redirecionamento do stdin para o extremo de leitura do pipe
                    dup2(pipes[breadth - 1][0], STDIN_FILENO);
                    close(pipes[breadth - 1][0]);

                    //Redirecionamento do stdout para o extremo de escrita criado
                    dup2(pipes[breadth][1], STDOUT_FILENO);
                    close(pipes[breadth][1]);

                    //Execucao do comando
                    execvp(getCommand(p->commands,breadth),getArgs(p->commands,breadth));
                }
                else{
                    //Fecha o extremo de escrita do pipe
                    close(pipes[breadth - 1][0]);
                    close(pipes[breadth][1]);
                }
            }
        }

        //Espera que todos os comandos acabem de executar
		
		for(breadth = 0; breadth < commandsCount ; breadth++) wait(NULL);
		int server_fifo,write_fifo,read_fifo;
		char writefifo[256];
		char readfifo[256];
		int pid = getpid();
		char pid_name[256];
		itoa(pid,pid_name);

		strcpy(writefifo,"./tmp/w_");
		strcat(writefifo,pid_name);

		// Criar os fifos para o cliente
		if(mkfifo(writefifo,0666)==-1){
			perror("Criacao fifo escrita");
			return 1;
		}

		strcpy(readfifo,"./tmp/r_");
		strcat(readfifo,pid_name);

		if(mkfifo(readfifo,0666)==-1){
			perror("Criacao fifo leitura");
			return 1;
		}

		// Abrir o fifo do servidor para escrever o pid do cliente
		if((server_fifo = open("./tmp/server_fifo", O_WRONLY)) == -1){
			perror("server fifo");
			return 1;
		}

		// Escrever o pid do cliente no fifo do servidor
		if (write(server_fifo,&pid,sizeof(int))==-1){
			perror("Write Pid Fifo");
			return 1;
		}

		close(server_fifo);
		if((write_fifo = open(writefifo,O_WRONLY)) == -1){
			perror("fifo escrita");
			return 1;
		}

		if((read_fifo = open(readfifo,O_RDONLY)) == -1){
			perror("fifo leitura");
			return 1;
		}
		// p:task_number
		char message[256];
		strcpy(message,"p:");
		char task_number[256];
		itoa(p->task_number,task_number);
		strcat(message,task_number);
		if(write(write_fifo,message,strlen(message))==-1){
			perror("Write Done");
			return 1;
		}

		//p:task
		// Fechar descritores fifos
		close(write_fifo);
		close(read_fifo);
		unlink(writefifo);
		unlink(readfifo);
	_exit(0);
	}
	else return pid_child;
}


int manageQueue(int pip[2]){

	if (fork()==0){
		LinkedListProcess processing = NULL;
		LinkedListProcess pending = NULL;
		LinkedListProcess done = NULL;	

		int process_id = 0;
		
		int pid_client;
		while(is_open){

		if (read(pip[0],&pid_client,sizeof(int))==-1){
			perror("Read pipe in addQueue");
			_exit(1);
		}
			
		char client_fifo[256];
		char pid_name[256];
							
		// Transformar pid do cliente numa string
		itoa(pid_client,pid_name);
		strcpy(client_fifo,"./tmp/w_");
		strcat(client_fifo,pid_name);
			
		// Criar fifos para o cliente
		int read_client_fifo = open(client_fifo,O_RDONLY);
		if(read_client_fifo == -1){
			perror("fifo de leitura");
			return 1;
		}
			
		strcpy(client_fifo,"./tmp/r_");
		strcat(client_fifo,pid_name);
		int write_client_fifo = open(client_fifo,O_WRONLY);
		if(write_client_fifo == -1){
			perror("fifo de escrita");
			return 1;
		}
				
		// Ler mensagem do cliente
		int size = read(read_client_fifo,buff,BUFFER_SIZE);
		if (size == -1){
			perror("Read buff");
			return 1;
		}
		buff[size] = '\0';
		printf("Mensagem : %s\n",buff);
		process_id++;

		// output file

		if (buff[0]=='p'){
			
			// p:task_number
			int done_task = atoi(&buff[2]);
			LinkedListProcess tmp = removeProcessByTaskNumber(processing,done_task);
			if (done == NULL) done = tmp;
			else for(LinkedListProcess tmp2 = processing;tmp2->next!=NULL;tmp2=tmp2->next) tmp2->next = tmp;
			if(pending !=NULL){
				tmp = removeProcessesHead(pending);

				if (processing == NULL) processing = tmp;
				else for(LinkedListProcess tmp2 = processing;tmp2->next!=NULL;tmp2=tmp2->next) tmp2->next = tmp;
				current_processing++;
				process(tmp);
			}


			// situacao de processing child escrever para o fifo

		}
		else{ 
			LinkedListProcess p = parseProcess(buff,pid_client,strlen(output_path)+strlen(pid_name),process_id);
			strcpy(p->output_file,output_path);
			strcat(p->output_file,pid_name);
			if (current_processing < max_process){
				if (processing == NULL) processing = p;
				else for(LinkedListProcess tmp = processing;tmp->next!=NULL;tmp=tmp->next) tmp->next = p;
				current_processing++;
				process(p);

			}else{
		
				if (pending == NULL) {
					pending = p;
				}else{
					appendsProcess(pending,p);
				}

				printProcessInfo(1,pending);
	
				wait(NULL);

			}
		}
		}
		freeProcess(pending);
		freeProcess(processing);
		freeProcess(done);

	}
	return 0;
}

int main(int argc, char* argv[]){
	
	// Verificar argumentos
	if (argc != 4) {
		perror("Argumentos");
		return -1;
	}

	printf("Starting server...\nPid : %d\n",getpid());

	// Guardar o caminho para a pasta onde vao ser guardados os outputs
	output_path = argv[1];
	max_process = atoi(argv[2]);

	// Criar fifo para o servidor
	if(mkfifo("./tmp/server_fifo",0666)==-1){
	 	perror("Fifo server");
	 	return -1;
	 }

	// Indicar a funcao que vai tratar do sinal sigterm
	signal(SIGTERM,handle_signalterm);
	signal(SIGINT,handle_signalint);

	// Abrir fifo para o servidor
	int server_fd = open("./tmp/server_fifo",O_RDONLY);
	if(server_fd == -1){
		perror("server fifo");
		return 1;
	}

	// Criar semaforo para os pending

	// filho que vai tratar de passar os pending para processing
	

	int pid_client;
	int pip[2];
	pipe(pip);

	manageQueue(pip);
	//close(pip[0]);
	while(is_open){
		// Ler fifo para onde o cliente escreve
		if(read(server_fd,&pid_client,sizeof(int))==-1){
			perror("Read client pid");
			return 1;
		}
		if(write(pip[1],&pid_client,sizeof(int))== -1){
		 	perror("Write pid");
		 	return 1;
		}
	}
	
	// Fechar descritor fifos
	close(server_fd);

	return 0;
}