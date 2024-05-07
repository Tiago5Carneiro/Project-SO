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

	// Eliminar fifos
	unlink(client_fifo);

	strcpy(client_fifo,"./tmp/r_");
	strcat(client_fifo,pid_name);

	// Eliminar fifos
	unlink(client_fifo);
	
	// Sair
	_exit(0);
}

// Funcao para lidar com Ctrl + C
void handle_signalint(){

	printf("\n\nUNEXPECTED TERMINATION \n\n");

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
int send_status(char * client_fifo){

	// Escrever no fifo do servidor para ele saber que o cliente quer o status
	printf("Getting status ...\n");

	// Escrever status no fifo do servidor
	if(write(write_fifo,"status",sizeof(char)*6) == -1){
		perror("escrever status");
		_exit(1);
	}

	if((read_fifo = open(client_fifo,O_RDONLY)) == -1){
		perror("fifo leitura");
		_exit(1);
	}

	int n;

	// Ler o status do fifo do cliente
	while((n = read(read_fifo, &buff, BUFF_SIZE)) >= 0){
		
		if (n!=0){
		if(write(1,&buff,n)== -1){
		 	perror("Write pid");
		 	_exit(1);
		}
		}

	}
	
	return 0;
} 

// Funcao para executar um comando
int execute(char* command,int size,char * client_fifo){

	// Escrever no fifo do servidor para ele saber que o cliente quer executar um comando
	printf("Executing ...\n");
	
	// Escrever o comando no fifo do servidor
	if(write(write_fifo,command,size) == -1){
		perror("execute write command");
		_exit(1);
	}

	close(write_fifo);

	if((read_fifo = open(client_fifo,O_RDONLY)) == -1){
		perror("fifo leitura");
		_exit(1);
	}
	// Ler o comando do fifo do cliente
	int ex = read(read_fifo,buff,BUFF_SIZE);
	if(ex == -1){
		perror("execute read command");
		_exit(1);
	}

	// Escrever o comando no stdout
	printf("%s\n",buff);

	close(read_fifo);

	return 0;
}

int main(int argc, char* argv[]){

	signal(SIGINT,handle_signalint);
	signal(SIGTERM,handle_signalterm);

	char client_fifo[256];
	char client_fifo2[256];

	// Criar os fifos para o cliente
	int pid = getpid();
	char pid_name[256];
	itoa(pid,pid_name);

	strcpy(client_fifo,"./tmp/w_");
	strcat(client_fifo,pid_name);

	if(mkfifo(client_fifo,0666)==-1){
		perror("Criacao fifo escrita");
		_exit(1);
	}

	strcpy(client_fifo2,"./tmp/r_");
	strcat(client_fifo2,pid_name);

	if(mkfifo(client_fifo2,0666)==-1){
		perror("Criacao fifo leitura");
		_exit(1);
	}

	// Abrir o fifo do servidor para escrita
	if((server_fifo = open("./tmp/server_fifo", O_WRONLY)) == -1){
		perror("server fifo");
		_exit(1);
	}

	// Escrever o pid do cliente no fifo do servidor
	if (write(server_fifo,&pid,sizeof(int))==-1){
		perror("Write Pid Fifo");
		_exit(1);
	}

	// Fechar o fifo do servidor
	close(server_fifo);

	// Abrir os fifos do cliente para leitura e escrita
	if((write_fifo = open(client_fifo,O_WRONLY)) == -1){
		perror("fifo escrita");
		_exit(1);
	}

	bool type=0;

	// Verificar se o argumento é execute ou status
	(strcmp(argv[1],"execute")==0) ? type = 1 : ((strcmp(argv[1],"status")==0) ? type = 0 : perror("Arguments"));

	if (type) {
		// Execute
    	char *mode = argv[3];

		// Verificar se o modo é válido
        if(!(strcmp(mode, "-u") || strcmp(mode, "-p"))){    
			perror("args");
			_exit(1);
		}

		// Calcular o tamanho total da string
		int total_size = 0;
		for(int i = 1; i < argc; i++) {
    		total_size += strlen(argv[i]) + 1;
		}

		// Criar a string para os argumentos
		char *args_string;
		args_string = (char *) malloc(total_size);
		if(args_string == NULL) {
    		perror("malloc");
    		_exit(1);
		}

		// Copiar o primeiro argumento
		strcpy(args_string, argv[1]);

		// Concatenar os argumentos
		for(int i = 2; i < argc; i++) {
			strcat(args_string, " ");	
    		strcat(args_string, argv[i]);
		}
		printf("String : %s\n", args_string);

		if((read_fifo = open(client_fifo,O_RDONLY)) == -1){
			perror("fifo leitura");
			_exit(1);
		}

		execute(args_string,total_size,client_fifo2);
	
	}else{
		// Status
    	send_status(client_fifo2);
    }

	// Eliminar fifos
    strcpy(client_fifo,"./tmp/w_");
	strcat(client_fifo,pid_name);

	unlink(client_fifo);

	strcpy(client_fifo,"./tmp/r_");
	strcat(client_fifo,pid_name);

	unlink(client_fifo);

	return 0;
}