#include <unistd.h> /* chamadas ao sistema: defs e decls essenciais */
#include <fcntl.h>  /* O_RDONLY, O_WRONLY, O_CREAT, O_APPEND, O_RDWR , O_TRUNC, O_EXCL, O_* */
#include <stdio.h>  /* printf */
#include <signal.h> /* signals */
#include <sys/stat.h> 	/* mkfifo */
#include <stdbool.h>
#include <string.h> 
#include <stdlib.h>

#define BUFF_SIZE 1024
char buff[BUFF_SIZE] ;

int server_fifo,
	read_fifo,
	write_fifo;

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

// Funcao para lidar com a terminacao do cliente
void handle_signalterm(){
	printf("Task done...\n");
	
	// Eliminar fifos
	char client_fifo[256];
	int pid = getpid();
	char pid_name[256];
	itoa(pid,pid_name);

	strcpy(client_fifo,"./tmp/w_");
	strcat(client_fifo,pid_name);

	unlink(client_fifo);

	strcpy(client_fifo,"./tmp/r_");
	strcat(client_fifo,pid_name);

	unlink(client_fifo);
	
	_exit(0);
}

// Funcao para lidar com Ctrl + C
void handle_signalint(){

	printf("\n\nUNEXPECTED TERMINATION \n\n");

	// Eliminar fifos
	char client_fifo[256];
	int pid = getpid();
	char pid_name[256];
	itoa(pid,pid_name);

	strcpy(client_fifo,"./tmp/w_");
	strcat(client_fifo,pid_name);

	unlink(client_fifo);

	strcpy(client_fifo,"./tmp/r_");
	strcat(client_fifo,pid_name);

	unlink(client_fifo);

	// Colocar o handler para o sinal int (Ctrl + C) de volta para a funcao default do kernel (SIG_DFL)
	signal(SIGINT,SIG_DFL);

	// Mandar um singal sig int para si novamente 
	kill(getpid(),SIGINT);
}

// Funcao para escrever o status para o fifo do cliente 
int send_status(){

	printf("Getting status ...\n");

	// Escrever status
	if(write(write_fifo,"status",sizeof(char)*6) == -1){
		perror("escrever status");
		return 1;
	}

	// Ficar a espera da resposta de volta do servidor que ele vai mandar pelo seu fifo
	int rd_fifo = read(read_fifo,buff,BUFF_SIZE);
	if(rd_fifo == -1){
		perror("fifo leitura");
		return 1;
	}
	buff[rd_fifo] = '\0';

	printf("Message : %s\n",buff);
	return 0;
} 

// Funcao para escrever o execute para o fifo do cliente 
int execute(char* command){

	printf("Executing ...\n");
	
	// Escrever no fifo para o servidor receber
	if(write(write_fifo,command,sizeof(command)) == -1){
		perror("execute");
		return 1;
	}

	// Esperar pela resposta do servidor que vai ser mandada pelo seu fifo
	int ex = read(server_fifo,buff,BUFF_SIZE);
	if(ex == -1){
		perror("execute");
		return 1;
	}

	printf("%s\n",buff);

	return 0;
}

int main(int argc, char* argv[]){

	signal(SIGINT,handle_signalint);
	signal(SIGTERM,handle_signalterm);

	// String que vai conter o path e nome do fifo de escrita do cliente
	char client_fifo[256];
	char client_fifo2[256];
	// Colucar o pid numa string para depois usar para criar os seus fifos respetivos
	int pid = getpid();
	char pid_name[256];
	itoa(pid,pid_name);


	strcpy(client_fifo,"./tmp/w_");
	strcat(client_fifo,pid_name);

	if(mkfifo(client_fifo,0666)==-1){
		perror("Criacao fifo escrita");
		return 1;
	}

	strcpy(client_fifo2,"./tmp/r_");
	strcat(client_fifo2,pid_name);

	if(mkfifo(client_fifo2,0666)==-1){
		perror("Criacao fifo leitura");
		return 1;
	}

	// Abertura dos fifos 
	if((server_fifo = open("./tmp/server_fifo", O_WRONLY)) == -1){
		perror("server fifo");
		return 1;
	}

	if (write(server_fifo,&pid,sizeof(int))==-1){
		perror("Write Pid Fifo");
		return 1;
	}

	if((write_fifo = open(client_fifo,O_WRONLY)) == -1){
		perror("fifo escrita");
		return 1;
	}

	if((read_fifo = open(client_fifo2,O_RDONLY)) == -1){
		perror("fifo leitura");
		return 1;
	}

	bool type=0;

	
	// Verificar se e um execute ou um status
	(strcmp(argv[1],"execute")==0) ? type = 1 : ((strcmp(argv[1],"status")==0) ? type = 0 : perror("Arguments"));

	if (type) {
		// Execute
		// time - int
		// mode = {p,u}
    	// u - comando individual
    	// p - pipeline de comandos
    	char mode = argv[3];

    	//verificacão se é um dos modos predefinidos
        if(!(strcmp(mode, "-u") || strcmp(mode, "-p"))){    
			perror("args");
			return 1;
		}

		// calcular o tamanho total necessário para a string
		int total_size = 0;
		for(int i = 1; i < argc; i++) {
    		total_size += strlen(argv[i]) + 1;
		}

		// criar a string com os argumentos do execute até ao fim do argv
		char *args_string = malloc(total_size);
		if(args_string == NULL) {
    		perror("malloc");
    		return 1;
		}

		// inicializar a string com o execute
		strcpy(args_string, argv[1]);

		// concatenar os argumentos restantes até ao fim
		for(int i = 2; i < argc; i++) {
    		strcat(args_string, " ");
    		strcat(args_string, argv[i]);
		}

		execute(args_string);
    }
    else{
    	// Status
    	send_status();
    }    

    strcpy(client_fifo,"./tmp/w_");
	strcat(client_fifo,pid_name);

	unlink(client_fifo);

	strcpy(client_fifo,"./tmp/r_");
	strcat(client_fifo,pid_name);

	unlink(client_fifo);


	return 0;
}