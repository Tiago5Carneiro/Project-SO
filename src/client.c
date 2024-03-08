#include <unistd.h> /* chamadas ao sistema: defs e decls essenciais */
#include <fcntl.h>  /* O_RDONLY, O_WRONLY, O_CREAT, O_APPEND, O_RDWR , O_TRUNC, O_EXCL, O_* */
#include <stdio.h>  /* printf */
#include <signal.h> /* signals */
#include <stdbool.h>
#include <string.h> 

#define BUFF_SIZE 1024
char buff[BUFF_SIZE] ;

int server_fifo,
	client_fifo;

void handle_usr1(){

	printf("Done executing\n");

}

int status(int server_fifo,int client_fifo){

	printf("Getting status ...\n");

	write(client_fifo,"status",sizeof(char)*6);

	read(server_fifo,buff,BUFF_SIZE);

	printf("%s\n",buff);
	return 0;
} 


int execute(int client_fifo, char* command){

	printf("Executing ...\n");
	
	// Escrever no fifo para o servidor receber
	write(client_fifo,command,sizeof(command));

	read(server_fifo,buff,BUFF_SIZE);

	printf("%s\n",buff);

	// Mandar sinal USR1 para si mesmo
	kill(getpid(),SIGUSR1);

	return 0;
}

int main(int argc, char* argv[]){

	// Abertura dos fifos 
	client_fifo = open("./tmp/client_fifo", O_WRONLY);
	server_fifo = open("./tmp/server_fifo", O_RDONLY);

	// Definir a funcao que da handle ao sinal SIGUSR1
	signal(SIGUSR1, handle_usr1);

	bool type=0;

	// Verificar se e um execute ou um status
	(strcmp(argv[1],"execute")==0) ? type = 1 : ((strcmp(argv[1],"status")==0) ? type = 0 : perror("Arguments"));

	if (type) {
		// Execute
		
		// mode = {p,u}
    	// u - comando individual
    	// p - pipeline de comandos
    	char mode = argv[2][1];
    
    	//switch para o mode selecionado
        switch (mode) {    
            case 'p':
                execute(client_fifo,argv[3]);
                break;
    
            default:
                printf("Invalid mode\n");
                return 1;
        }
    }
    else{
    	// Status
    	status(server_fifo,client_fifo);
    }    
	return 0;
}