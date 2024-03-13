#include <unistd.h> 	/* chamadas ao sistema: defs e decls essenciais */
#include <fcntl.h>  	/* O_RDONLY, O_WRONLY, O_CREAT, O_APPEND, O_RDWR , O_TRUNC, O_EXCL, O_* */
#include <stdlib.h> 	/* Malloc */
#include <stdio.h>  	/* printf */
#include <signal.h> 	/* signals (SIGTERM, SIGALARM)*/
#include <sys/stat.h> 	/* mkfifo */
#include <sys/wait.h> 	/* wait */
#include <sys/types.h>


#define BUFFER_SIZE 1024

static volatile int is_open = 1;
char* output_path;
char buff[BUFFER_SIZE];

// ./orchestrator {output_folder} {parallel-tasks} {sched-policy}

void handle_signalterm(){
	printf("Closing server...\n");
	
	// Eliminar fifos
	unlink("./tmp/client_fifo");
	unlink("./tmp/server_fifo");
	
	_exit(0);
}

void handle_signalint(){

	printf("\n\nUNEXPECTED TERMINATION \n\n");

	// Eliminar fifos
	unlink("./tmp/client_fifo");
	unlink("./tmp/server_fifo");

	// Colocar o handler para o sinal int (Ctrl + C) de volta para a funcao default do kernel (SIG_DFL)
	signal(SIGINT,SIG_DFL);

	// Mandar um singal sig int para si novamente 
	kill(getpid(),SIGINT);
}

int read_child(){

}

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
	if(mkfifo("./tmp/server_fifo",0644)==-1){
	 	perror("Fifo server");
	 	return -1;
	 }
 	if(mkfifo("./tmp/client_fifo",0644)==-1){
	 	perror("Fifo server");
	 	return -1;
	 }

	// Indicar a funcao que vai tratar do sinal sigterm
	signal(SIGTERM,handle_signalterm);
	signal(SIGINT,handle_signalint);

	// // Descritores para os fifos
	int client_fd = open("./tmp/client_fifo",O_RDONLY);
	int server_fd = open("./tmp/server_fifo",O_WRONLY);
	int status;
	while(is_open){
		
		// Ler fifo para onde o cliente escreve
		read(client_fd,buff,BUFFER_SIZE);
		if(fork()==0){
			
			// filho que vai tratar da informacao providenciada pelo cliente
			printf("%s\n",buff);

			// filho informa cliente que o seu pedido ja foi concluido
			write(server_fd,"done",sizeof(char)*4);

			// filho termina 
			_exit(0);

		}
	}
	// Fechar descritor fifos
	close(client_fd);
	close(server_fd);
		
	// Eliminar fifos
	unlink("./tmp/client_fifo");
	unlink("./tmp/server_fifo");
	
	return 0;
}