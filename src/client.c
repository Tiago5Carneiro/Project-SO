#include <unistd.h> /* chamadas ao sistema: defs e decls essenciais */
#include <fcntl.h>  /* O_RDONLY, O_WRONLY, O_CREAT, O_APPEND, O_RDWR , O_TRUNC, O_EXCL, O_* */
#include <stdio.h>  /* printf */
#include <signal.h> /* signals */
#include <sys/stat.h> 	/* mkfifo */
#include <stdbool.h>
#include <string.h> 

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
	char client_fifo[26];
	int pid = getpid();
	char pid_name[19];
	itoa(pid,pid_name);

	strcpy(client_fifo,"./tmp/w_");
	strcpy(client_fifo+8,pid_name);

	unlink(client_fifo);

	strcpy(client_fifo,"./tmp/r_");
	strcpy(client_fifo+8,pid_name);

	unlink(client_fifo);
	
	_exit(0);
}

// Funcao para lidar com Ctrl + C
void handle_signalint(){

	printf("\n\nUNEXPECTED TERMINATION \n\n");

	// Eliminar fifos
	char client_fifo[26];
	int pid = getpid();
	char pid_name[19];
	itoa(pid,pid_name);

	strcpy(client_fifo,"./tmp/w_");
	strcpy(client_fifo+8,pid_name);

	unlink(client_fifo);

	strcpy(client_fifo,"./tmp/r_");
	strcpy(client_fifo+8,pid_name);

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
	write(write_fifo,"status",sizeof(char)*6);

	// Ficar a espera da resposta de volta do servidor que ele vai mandar pelo seu fifo
	read(read_fifo,buff,BUFF_SIZE);

	printf("Message : %s\n",buff);
	return 0;
} 

// Funcao para escrever o execute para o fifo do cliente 
int execute(int client_fifo, char* command){

	printf("Executing ...\n");
	
	// Escrever no fifo para o servidor receber
	write(write_fifo,command,sizeof(command));

	// Esperar pela resposta do servidor que vai ser mandada pelo seu fifo
	read(server_fifo,buff,BUFF_SIZE);

	printf("%s\n",buff);

	return 0;
}

int main(int argc, char* argv[]){

	signal(SIGINT,handle_signalint);
	signal(SIGTERM,handle_signalterm);

	// Abertura dos fifos 
	if((server_fifo = open("./tmp/server_fifo", O_WRONLY)) == -1){
		perror("server fifo");
		return 1;
	}

	// Colucar o pid numa string para depois usar para criar os seus fifos respetivos
	int pid = getpid();
	char pid_name[19];
	itoa(pid,pid_name);

	if (write(server_fifo,&pid,sizeof(int))==-1){
		perror("Write Pid Fifo");
		return 1;
	}

	// String que vai conter o path e nome do fifo de escrita do cliente
	char client_fifo[26];
	printf("Pid : %s\n",&pid_name);
	strcpy(client_fifo,"./tmp/w_");
	strcpy(client_fifo+8,pid_name);

	if(mkfifo(client_fifo,0666)==-1){
		perror("Criacao fifo escrita");
		return 1;
	}

	if((write_fifo = open(client_fifo,O_WRONLY)) == -1){
		perror("fifo escrita");
		return 1;
	}

	strcpy(client_fifo,"./tmp/r_");
	strcpy(client_fifo+8,pid_name);

	if(mkfifo(client_fifo,0666)==-1){
		perror("Criacao fifo leitura");
		return 1;
	}

	if((read_fifo = open(client_fifo,O_RDONLY)) == -1){
		perror("fifo leitura");
		return 1;
	}

	bool type=0;

	/*
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
                break;
    
            default:
                printf("Invalid mode\n");
                return 1;
        }
    }
    else{
    	// Status
    	send_status();
    }    
	
	*/
	send_status();
    strcpy(client_fifo,"./tmp/w_");
	strcpy(client_fifo+8,pid_name);

	unlink(client_fifo);

	strcpy(client_fifo,"./tmp/r_");
	strcpy(client_fifo+8,pid_name);

	unlink(client_fifo);


	return 0;
}