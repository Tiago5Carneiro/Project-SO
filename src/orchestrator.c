#include <unistd.h> 	/* chamadas ao sistema: defs e decls essenciais */
#include <fcntl.h>  	/* O_RDONLY, O_WRONLY, O_CREAT, O_APPEND, O_RDWR , O_TRUNC, O_EXCL, O_* */
#include <stdlib.h> 	/* Malloc */
#include <stdio.h>  	/* printf */
#include <signal.h> 	/* signals (SIGTERM, SIGALARM)*/
#include <sys/stat.h> 	/* mkfifo */
#include <sys/wait.h> 	/* wait */
#include <sys/types.h>
#include <string.h>

/*

	Ideas on how to do things : 

	Cliente vai mandar a string toda que quer executar 

	Server verifica se recebeu status ou exectute

		Se status verificar logo como esta a queue e mandar a informacao para o cliente, depois mandar 
		a informacao da queue que contem os commandos que estao a ser executados agora 
			Uma ideia seria mandar strings individuais por processo em espera e por processo a ser executado 
			Parar de mandar atravez de um character especifico e assim que o cliente receber este character 
			parar de ler do fifo

		Se execute for recebido 
			verificar se foi usado p ou u
				se u :
					fazer um filho que vai usar o execvp para executar o commando dado 
					direcionar o stdout para um ficheiro no path dado como argumento ao servidor 
				se p :
					usar o strstep para encontrar o caracter '|' e por cada vez que encontrar ou quando encontrar \0, 
					criar filho para executar cada um destes commandos 
					cada filho direciona o stdout para um ficheiro que ira para o path 

		Ideia de como gerir a queue 
			Processo pai do programa vai ser o processo que esta sempre a ler do fifo 
			Este cria um filho logo no inicio, que vai ser quem gere a queue 
			Este filho vai ficar a ler dum pipe vindo do pai ate receber informacao para criar um processo para a queue 
			Quando ler um novo pedido vai verificar na queue de processos a serem processados neste momento se pode ser 
			ja iniciado este pedido 
			Se sim cria logo um filho para comecar a tratar desse processo e adiciona o processo a queue dos que estao 
			a ser executados
			Otherwise vai para a queue dos de espera
			
		Como fazer ir da queue de espera para ser processado
			Quando o filho terminar de processar entao de alguma maneira um processo deveria verificar se o proximo pro
			cesso da queue ja pode ser feito 

*/

#define BUFFER_SIZE 1024

static volatile int is_open = 1;
char* output_path;
char buff[BUFFER_SIZE];

pid_t pids[50];

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

	// Indicar a funcao que vai tratar do sinal sigterm
	signal(SIGTERM,handle_signalterm);
	signal(SIGINT,handle_signalint);

	// // Descritores para os fifos
	int server_fd = open("./tmp/server_fifo",O_RDONLY);
	
	pid_t pid;
	int i;
	int status;
	while(is_open){
		
		// Ler fifo para onde o cliente escreve
		read(server_fd,&pid,sizeof(pid_t));
		i = 0;
		while(pids[i]!=0){ 
			i++;
		}
		pids[i] = pid;
		if(fork()==0){
			
			// filho que vai tratar da informacao providenciada pelo cliente
			
			// Eliminar fifos
			char client_fifo[26];
			char pid_name[19];
			itoa(pid,pid_name);

			strcpy(client_fifo,"tmp/w_");
			strcpy(client_fifo+7,pid_name);

			int read_client_fifo = open(client_fifo,O_RDONLY);

			strcpy(client_fifo,"tmp/r_");
			strcpy(client_fifo+7,pid_name);

			int write_client_fifo = open(client_fifo,O_WRONLY);

			read(read_client_fifo,buff,BUFFER_SIZE);
			
			printf("Mensagem : %s\n",buff);

			// filho informa cliente que o seu pedido ja foi concluido
			write(write_client_fifo,"Done",sizeof(char)*4);

			close(write_client_fifo);
			close(read_client_fifo);

			// filho termina 
			_exit(0);

		}
		wait(&status);
		is_open = 0;
	}
	// Fechar descritor fifos
	close(server_fd);
		
	// Eliminar fifos
	unlink("./tmp/server_fifo");
	
	return 0;
}