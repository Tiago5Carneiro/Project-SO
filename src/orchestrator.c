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

#define BUFFER_SIZE 1024

static volatile int is_open = 1;
char* output_path;
char buff[BUFFER_SIZE];

//Command processing;
//Command pending;

pid_t pids[256];

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

// adicionar a queue 
int addQueue(char * task, int pid_client, char* pid_name){
	// output file
	LinkedListProcess p = parseProcess(buff,pid_client);
	p->output_file = malloc(strlen(output_path)+strlen(pid_name)+1);
	strcpy(p->output_file,output_path);
	strcat(p->output_file,pid_name);
	printf("Output file : %s\n",p->output_file);
	//printProcessInfo(1,p);
	//write(1,"OLA",4);
	//for(int i = 0;hello[i] != NULL;i++)printf("args : %s",-args[i]);
	freeProcess(p);
	//int fd = open(p->output_file,O_WRONLY | O_CREAT | O_TRUNC,0666);
	//dup2(1,fd);

	/*
	// filho que vai executar o comando
	// verificar se é u ou p
	printf("OLA/n");
	char* distinct;
	int j = 0;
	// passar o exec
	while(buff[j] != ' '){
			j++;
	}
	j++;
	// passar o tempo
	while(buff[j] != ' '){
		j++;
	}
	j++;
	distinct = &buff[j];

	if(distinct[1] == 'u'){
		// executar comando unico
		char *args[256];
		int i = 1;
		int j=0;
		int z=0;
		//passar o -u
		while(distinct[j] != ' '){
			j++;
		}
		j++;
		z = j;
		// ver quantidade de characteres ate o proximo espaco
		while(distinct[j] != ' '){
			j++;
		}
		j++;
		// colocar o nome do commando no arg[0]
		args[0] = malloc(sizeof(char)*(j-z));
		strncpy(args[0],&distinct[z],j-z);
		z=j;
		// funcao para colocar os argumentos do commando nos respectivos args
		while(j<strlen(distinct)+1){
			if (distinct[j]==' ' || distinct[j]=='\0') {
				args[i] = malloc(sizeof(char)*(j-z));
				strncpy(args[i],&distinct[z],j-z);
				z = j + 1;
				//printf("arg: %s ", args[i]);
				i++;
			 if (distinct[j]=='\0') {
				args[i] = NULL;
				}
		
			j++;
		}
		printf("Command : %s\n" , args[0]);
		//for (int x = 0;args[x] == NULL;x++)printf("arg: %s ", args[x]);
		printf("%s\n",args);
		execvp(args[0],args);
		perror("execvp");
		_exit(1);
	}
	}else if(distinct[1] == 'p'){
		//executar comando pipe
	}else{
		// status
	}
	*/
	return 0;
}

// passar para os processing


int main(int argc, char* argv[]){

	// Verificar se tem 3 argumentos
	if (argc != 4) {
		perror("Argumentos");
		return -1;
	}

	printf("Starting server...\nPid : %d\n",getpid());

	// Path para onde os ficheiros finais vao	
	output_path = argv[1];

	// Make fifos
	if(mkfifo("./tmp/server_fifo",0666)==-1){
	 	perror("Fifo server");
	 	return -1;
	 }

	// Indicar a funcao que vai tratar do sinal sigterm
	signal(SIGTERM,handle_signalterm);
	signal(SIGINT,handle_signalint);

	// // Descritores para os fifos
	int server_fd = open("./tmp/server_fifo",O_RDONLY);
	if(server_fd == -1){
		perror("server fifo");
		return 1;
	}

	int pid_client;
	int i;
	int status;
		
		// Ler fifo para onde o cliente escreve
		if(read(server_fd,&pid_client,sizeof(int))==-1){
			perror("Read client pid");
			return 1;
		}

		if(fork()==0){
			
			// filho que vai tratar da informacao providenciada pelo cliente
			
			// Eliminar fifos
			char client_fifo[256];
			char pid_name[256];
			
			// Transformar pid do cliente numa string
			itoa(pid_client,pid_name);
			strcpy(client_fifo,"./tmp/w_");
			strcat(client_fifo,pid_name);

			printf("Pid cliente : %s\n",pid_name);

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

			int size = read(read_client_fifo,buff,BUFFER_SIZE);
			if (size == -1){
				perror("Read buff");
				return 1;
			}
			buff[size] = '\0';
			printf("Mensagem : %s\n",buff);

			// verificar se o cliente quer status ou executar um comando
			if(buff[0] == 's'){
				// cliente quer status

			}else{
				//cliente quer executar um comando
				if(fork()==0){
					// filho que vai executar o comando
						/*
						if(fd == -1){
							perror("open file");
							_exit(1);
						}
						dup2(fd,1);
						executar comando
						*/
						addQueue(buff,pid_client,pid_name);
						_exit(0);
				}else{
					// pai que vai esperar pelo filho que executa o comando
					wait(&status);
				}
			}
			// filho informa cliente que o seu pedido ja foi concluido
			if(write(write_client_fifo,"Done",sizeof(char)*4)==-1){
				perror("Write Done");
				return 1;
			}

			// Fechar descritores fifos
			close(write_client_fifo);
			close(read_client_fifo);

			// filho termina 
			_exit(0);

		}else{
		wait(&status);
		is_open = 0;
		}
	//}else{
	//// pai que vai esperar pelo filho que trata da informacao do cliente
	//wait(&status);

	// Fechar descritor fifos
	close(server_fd);
		
	// Eliminar fifos
	unlink("./tmp/server_fifo");
	
	return 0;
}