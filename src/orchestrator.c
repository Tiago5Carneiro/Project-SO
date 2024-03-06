#include <sys/types.h>
#include <unistd.h> /* chamadas ao sistema: defs e decls essenciais */
#include <fcntl.h>  /* O_RDONLY, O_WRONLY, O_CREAT, O_APPEND, O_RDWR , O_TRUNC, O_EXCL, O_* */
#include <stdlib.h> /* Malloc */
#include <stdio.h>  /* printf */
#include <signal.h> /* signals (SIGTERM, SIGALARM)*/
#include <sys/stat.h> /* mkfifo */

#define BUFFER_SIZE 1024

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

	int status;
	int is_open = 1;
	while(is_open){
		
		// // Descritores para os fifos
		int client_fd = open("./tmp/client_fifo",O_RDONLY);
		int server_fd = open("./tmp/server_fifo",O_WRONLY);
		
		if(fork()==0){
			// filho
			read(client_fd,buff,BUFFER_SIZE);
			printf("%s\n",buff);
			_exit(0);
		}
		else{

			// Servidor espera pelo valor de saida do seu filho
			wait(&status);

			// Se valor de saida for igual a 0 fechar
			if (status == WEXITSTATUS(status)){
				if (status == 0) is_open = 0;
			}
		}

		// Fechar descritor fifos
		close(client_fd);
		close(server_fd);
		
		// Eliminar fifos
		unlink("./tmp/client_fifo");
		unlink("./tmp/server_fifo");
	}
	return 0;
}