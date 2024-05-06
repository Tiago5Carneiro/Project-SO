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
static volatile int max_process = 0;

char* output_path;
char buff[BUFFER_SIZE];

LinkedListProcess processing;
LinkedListProcess pending;
LinkedListProcess done;


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
	
	// Eliminar fifos
	unlink("./tmp/server_fifo");
	
	_exit(0);
}

// Funcao para lidar com Ctrl + C
void handle_signalint(){

	printf("\n\nUNEXPECTED TERMINATION \n\n");

	// Eliminar fifos
	unlink("./tmp/server_fifo");

	// Colocar o handler para o sinal int (Ctrl + C) de volta para a funcao default do kernel (SIG_DFL)
	signal(SIGINT,SIG_DFL);

	// Mandar um singal sig int para si novamente 
	kill(getpid(),SIGINT);
}

int process();

// adicionar a queue 
int addQueue(char * task, int pid_client, char* pid_name, int task_number){
	// output file
	LinkedListProcess p = parseProcess(buff,pid_client,strlen(output_path)+strlen(pid_name),task_number);
	strcpy(p->output_file,output_path);
	strcat(p->output_file,pid_name);
	printf("Output file : %s\n",p->output_file);

	printProcessInfo(1,p);

	//int fd = open(p->output_file,O_WRONLY | O_CREAT | O_TRUNC,0666);
	//dup2(fd,1);
	//execvp(p->commands->args[0],p->commands->args);
	freeProcess(p);

	return 0;
}

// passar para os processing


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
	if(fork()==0){

	}else{
		// pai
		while(is_open){
			//usar mutex nos pending
			if (pending!=NULL){
				//unmutex nos pending
				
				LinkedListProcess tmp;
				int i = 0;
				//mutex no processing
				for(tmp=processing;tmp!=NULL;tmp=tmp->next)i++;
				//unmutex nos processing
				if (i<max_process);// fazer a cabeca dos pending ir para os processing
				//else 
			}else{
				
			}
		}
	}
	int pid_client;
	int status;
	int pip[2];
	pipe(pip);
	// Criar um filho para tratar da queue pending
		if(fork()==0){
			int process_id;
			// fechar escrita pipe
			close(pip[1]);
			while(is_open){
				read(pip[0],&pid_client,sizeof(int));
			
				process_id++;
				char client_fifo[256];
				char pid_name[256];
							
				// Transformar pid do cliente numa string
				itoa(pid_client,pid_name);
				strcpy(client_fifo,"./tmp/w_");
				strcat(client_fifo,pid_name);
				
				printf("Pid cliente : %s\n",pid_name);
				
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
				
				addQueue(buff,pid_client,pid_name,process_id);
				// Enviar mensagem ao cliente a dizer que terminou
				if(write(write_client_fifo,"Task received",13)==-1){
					perror("Write Done");
					return 1;
				}
				
				// Fechar descritores fifos
				close(write_client_fifo);
				close(read_client_fifo);
			
							// filho termina
				}

			}else {
			close(pip[0]);
			}

	while(is_open){
		// Ler fifo para onde o cliente escreve
		if(read(server_fd,&pid_client,sizeof(int))==-1){
			perror("Read client pid");
			return 1;
		}
		write(pip[1],&pid_client,sizeof(int));
	}

	// Fechar descritor fifos
	close(server_fd);
		
	// Eliminar fifos
	unlink("./tmp/server_fifo");
	
	return 0;
}