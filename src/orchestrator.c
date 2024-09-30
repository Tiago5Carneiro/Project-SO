#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include "../include/auxStructs.h"
#include <semaphore.h>

#define BUFFER_SIZE 256

// ./orchestrator {output_folder} {parallel-tasks} {sched-policy}

// ./bin/client execute 500 -u "ls -l | cat" 
// the above works
// ./bin/client execute 500 -u "ls -l"
// the above does not work

static volatile int is_open = 1;
static volatile int current_processing = 0;
static volatile int max_process = 0;

LinkedListProcess processing = NULL;
LinkedListProcess pending = NULL;
LinkedListProcess done = NULL;

char* output_path;
char buff[BUFFER_SIZE];

int countDigit(long long n)
{
    if (n/10 == 0)
        return 1;
    return 1 + countDigit(n / 10);
}

// Flips a string in place
void flip(char* string) {
    int i, j;
    char aux;
    for (i = 0, j = strlen(string) - 1; i < j; i++, j--) {
        aux = string[i];
        string[i] = string[j];
        string[j] = aux;
    }
}

// Integer to ASCII conversion
void itoa(int n, char* string) {
    int i = 0, sign = n;
    if (n < 0) n = -n;
    do {
        string[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    if (sign < 0) string[i++] = '-';
    string[i] = '\0';
    flip(string);
}

// Handles server termination (SIGTERM)
void handle_signalterm() {
    printf("Closing server...\n");
    is_open = 0;
    unlink("./tmp/server_fifo");
}

// Handles unexpected termination (Ctrl+C)
void handle_signalint() {
    printf("\n\nUNEXPECTED TERMINATION \n\n");
    unlink("./tmp/server_fifo");
    signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
}

// Writes status to the client's FIFO
void status(LinkedListProcess p, int write_fifo) {
    printf("Write fifo status");

    if (fork() == 0) {

        // Write pending processes
        LinkedListProcess tmp;
        write(write_fifo, "Pending : \n", 12);
        for (tmp = pending; tmp != NULL; tmp = tmp->next) {
            write(write_fifo, "\n", 1);
            printProcessInfo(write_fifo, tmp);
        }

        // Write processing processes
        write(write_fifo, "Processing : \n", 15);
        for (tmp = processing; tmp != NULL; tmp = tmp->next) {
            write(write_fifo, "\n", 1);
            printProcessInfo(write_fifo, tmp);
        }

        // Write done processes
        write(write_fifo, "Done\n", 9);
        for (tmp = done; tmp != NULL; tmp = tmp->next) {
            write(write_fifo, "\n", 1);
            printProcessInfo(write_fifo, tmp);
        }

        close(write_fifo);
        _exit(0);
    }
    wait(NULL);
}

// Processes a command and sets up pipes if necessary
int process(LinkedListProcess p){
	int pid_child;

	// Criar o processo filho
	if ( (pid_child = fork()==0)){
		
        //Abertura do ficheiro no qual ficará o resultado final de aplicar os comandos
        int output_fp = open(p->output_file, O_CREAT | O_WRONLY | O_TRUNC , 0644);
        if(output_fp == -1) { 
			perror("Open Output file");
			_exit(1); 
			}
        int commandsCount = p->commandsCount;
        int breadth;
        int pipes[commandsCount - 1][2]; //Tendo N comandos, temos N - 1 pipes
		
	//Iterar sobre os comandos
        for(breadth = 0; breadth <= commandsCount ; breadth++){

			char ** args = getArgs(p->commands,breadth);
			char* command = args[0];
			//É o primeiro comando
            if(breadth == 0){
				
                //É o único comando
                if(commandsCount == 0){
                	
                    if(fork() == 0){
                        //Redirecionamento do stdout para o ficheiro de output fornecido
                        dup2(output_fp, STDOUT_FILENO);
                        close(output_fp);

                        //Execucao do comando
                        execvp(command,args);
                    }
                }
                //É o primeiro comando (Apenas quando tem mais do que um comando)
                else{

                    //Cria o pipe comando
                    if(pipe(pipes[0]) == -1) _exit(1);

					//Cria o processo filho
                    if(fork() == 0){

						//Fecha o extremo de leitura do pipe
                        close(pipes[0][0]);

                        //Redirecionamento do stdout para o extremo de escrita do pipe
                        dup2(pipes[0][1], STDOUT_FILENO);

                        //Execucao do comando
                        execvp(command,args);
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
                    execvp(command,args);
                }
                else{
                    //Fecha o extremo de leitura do pipe
                    close(pipes[breadth - 1][0]);

					//Fecha o ficheiro de output
                    close(output_fp);
                }
            }
            //É um comando intermédio
            else{
                //Cria o pipe comando
                if(pipe(pipes[breadth]) == -1) _exit(1);

				//Cria o processo filho
                if(fork() == 0){

                    //Redirecionamento do stdin para o extremo de leitura do pipe
                    dup2(pipes[breadth - 1][0], STDIN_FILENO);
                    close(pipes[breadth - 1][0]);

                    //Redirecionamento do stdout para o extremo de escrita do pipe
                    dup2(pipes[breadth][1], STDOUT_FILENO);
                    close(pipes[breadth][1]);

                    //Execucao do comando
                    execvp(command,args);
                }
                else{
                    //Fecha o extremo de leitura do pipe
                    close(pipes[breadth - 1][0]);

					//Fecha o extremo de escrita do pipe
                    close(pipes[breadth][1]);
                }
            }
        }

        //Esperar pelos processos filhos
		for(breadth = 0; breadth < commandsCount ; breadth++) wait(NULL);

		int server_fifo,write_fifo;
		char writefifo[256];

		// Obter o pid do processo
		int pid = getpid();
		char pid_name[256];

		// Transformar pid do cliente numa string
		itoa(pid,pid_name);

		snprintf(writefifo,256,"./tmp/w_%s",pid_name);

		// Criar fifos para o cliente
		if(mkfifo(writefifo,0666)==-1){
			perror("Criacao fifo escrita");
			_exit(1);
		}

		// Abrir o fifo do servidor para escrever o pid do cliente
		if((server_fifo = open("./tmp/server_fifo", O_WRONLY)) == -1){
			perror("server fifo");
			_exit(1);
		}
				
		// Escrever o pid do cliente no fifo do servidor
		if (write(server_fifo,&pid,sizeof(int))==-1){
			perror("Write Pid Fifo");
			_exit(1);
		}

		// Fechar descritor fifos
		close(server_fifo);

		// Abrir fifos para escrita e leitura
		if((write_fifo = open(writefifo,O_WRONLY)) == -1){
			perror("fifo escrita");
			_exit(1);
		}

		// p:task_number
		char message[256];

		char task_number[256];

		int digits = countDigit(p->task_number);
		// Transformar task number numa string
		itoa(p->task_number,task_number);

		snprintf(message,256,"p:%s",task_number);
		write(1,message,2 + digits);
		// Escrever mensagem para o servidor
		if(write(write_fifo,message,2+digits)==-1){
			perror("Write Done");
			_exit(1);
		}

		// Fechar descritores fifos
		close(write_fifo);
		unlink(writefifo);
	_exit(0);

	}
	return pid_child;
}

// Main function to start the server
int main(int argc, char* argv[]) {

	signal(SIGINT, handle_signalint);
    signal(SIGTERM, handle_signalterm);

    if (argc != 4) {
        perror("Argumentos");
        _exit(1);
    }

    printf("Starting server...\nPid : %d\n", getpid());
    output_path = argv[1];
    max_process = atoi(argv[2]);

    if (mkfifo("./tmp/server_fifo", 0666) == -1) {
        perror("Fifo server");
        _exit(1);
    }

    int server_fifo;
    

    int process_id = 0;
    int pid_client;
    int read_size;
    while ((server_fifo = open("./tmp/server_fifo", O_RDONLY))) {

    	if (server_fifo == -1) {
        	perror("Open server_fifo");
        	_exit(1);
    	}

    	if ((read_size = read(server_fifo, &pid_client, sizeof(int)))==-1){
    		perror("Read server_fifo");
        	_exit(1);
    	}

    	strcpy(buff,"");

        char client_fifo[256], pid_name[256];
        itoa(pid_client, pid_name);

        snprintf(client_fifo,256,"./tmp/w_%s",pid_name);

        int read_client_fifo = open(client_fifo, O_RDONLY);
        if (read_client_fifo == -1) {
            perror("fifo de leitura");
            _exit(1);
        }

        int size = read(read_client_fifo, buff, BUFFER_SIZE);
        if (size == -1) {
           perror("Read buff");
           _exit(1);
        }

        buff[size] = '\0';

        // Child processes that finished 
        if (buff[0] == 'p') {
        	read_size= read_size-2;
        	char * tasknumber = malloc(read_size);
        	strncpy(&buff[2],tasknumber,read_size);
            int done_task = atoi(tasknumber); // Get task id to remove
            write(1,tasknumber,read_size);
            free(tasknumber);
            printf("Task number : %d", done_task);
            LinkedListProcess tmp = removeProcessByTaskNumber(processing, done_task); // Remove task from processing

            appendsProcess(done, tmp); // Append process to the done queue
            if (done == NULL) done=tmp;
            write(1, "Done : ", 7);
            printProcessInfo(1, done);

            // Start next pending process if possible 
            if (pending != NULL) {

                tmp = removeProcessesHead(pending); // Get highest priority process 

                appendsProcess(processing,tmp); // Append to processing 

                itoa(pid_client, pid_name);
                snprintf(client_fifo,256,"./tmp/r_%s",pid_name);

           		int write_client_fifo = open(client_fifo, O_WRONLY);
           		if (write_client_fifo == -1) {
               		perror("fifo de escrita");
               		_exit(1);
           		}

                write(write_client_fifo, "Task accepted ...\n", 19);

                // Call status or process depending on which type of requrest it is
                if (strcmp(tmp->commands->args[0], "status")) {
                    close(write_client_fifo);
                    process(tmp);
                }
                   else status(tmp,write_client_fifo);
               }
               else current_processing--;

           } 
           // Request came from a client
           else{
            	snprintf(client_fifo,256,"./tmp/r_%s",pid_name);

            	int write_client_fifo = open(client_fifo, O_WRONLY);
            	if (write_client_fifo == -1) {
                	perror("fifo de escrita");
                	_exit(1);
            	}

                process_id++;
                LinkedListProcess p = parseProcess(buff, pid_client, strlen(output_path) + strlen(pid_name), process_id); // Parse request

                strcpy(p->output_file,output_path);
                strcat(p->output_file,pid_name);
                // Check if it is possible to start processing the current request 
                if (current_processing < max_process) {

                    appendsProcess(processing, p);
                    current_processing++;

                    write(1, "Processing : ", 13);
                    printProcessInfo(1, p);
                    write(write_client_fifo, "Task accepted ...\n", 19);

                    // Checking the type of request 
                    if (strcmp(p->commands->args[0], "status")) {
                    	close(write_client_fifo);
                    	process(p);
                    }
                    else status(p,write_client_fifo);
                } else appendsProcess(pending, p); // If not possible to start processing rn just add to the pending 
                
            }
            close(server_fifo);

    }
        // If out of the while loop, wait for all children then free up the queue's
	wait(NULL);
    freeProcess(pending);
    freeProcess(processing);
    freeProcess(done);
    close(server_fifo);
    unlink("./tmp/server_fifo");

    return 0;
}
