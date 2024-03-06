#include <unistd.h> /* chamadas ao sistema: defs e decls essenciais */
#include <fcntl.h>  /* O_RDONLY, O_WRONLY, O_CREAT, O_APPEND, O_RDWR , O_TRUNC, O_EXCL, O_* */
#include <stdio.h>  /* printf */
#include <signal.h> /* signals */


void handle_usr1(){

	printf("Done executing\n");

}

int execute(int client_fifo){

	printf("Executing ...\n");
	
	// Escrever no fifo para o servidor receber
	write(client_fifo,"Received\0",sizeof(char)*8);

	// Mandar sinal USR1 para si mesmo
	kill(getpid(),SIGUSR1);

	return 0;
}

int main(int argc, char* argv[]){

	// Abertura dos fifos 
	int client_fifo = open("./tmp/client_fifo", O_WRONLY);
	int server_fifo = open("./tmp/server_fifo", O_RDONLY);

	// Definir a funcao que da handle ao sinal SIGUSR1
	signal(SIGUSR1, handle_usr1);

	// mode = {p,u}
	// u - comando individual
	// p - pipeline de comandos
	char mode = argv[3][1];

	// switch para o mode selecionado
    switch (mode) {
        case 'p':
            execute(client_fifo);
            break;
        default:
            printf("Invalid mode\n");
            return 1;
    }
    
	return 0;
}