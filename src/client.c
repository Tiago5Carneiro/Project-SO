#include <unistd.h>   /* system calls */
#include <fcntl.h>    /* file control options */
#include <stdio.h>    /* standard I/O */
#include <signal.h>   /* signal handling */
#include <sys/stat.h> /* mkfifo */
#include <stdbool.h>
#include <string.h> 
#include <stdlib.h>

#define BUFF_SIZE 1024
char buff[BUFF_SIZE];

int server_fifo, read_fifo, write_fifo;

// ./client {execute|status} {time} {-u|-p} {"prog-a [args]"}

// Function to reverse a string
void flip(char* string) {
    int i = 0, j = strlen(string) - 1;
    char aux;

    while (i < j) {
        aux = string[i];
        string[i] = string[j];
        string[j] = aux;
        i++;
        j--;
    }
}

// Simple itoa function to convert integer to string
void itoa(int n, char* string) {
    int i = 0, sign = n;

    if (n < 0) n = -n; // Handle negative sign

    do { // Convert digits in reverse order
        string[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0) string[i++] = '-';
    string[i] = '\0';

    flip(string); // Reverse to correct order
}

// Signal handler for client termination
void handle_signalterm() {
    printf("Task done...\n");

    // Remove FIFOs
    char client_fifo[256];
    int pid = getpid();
    char pid_name[256];
    itoa(pid, pid_name);

    snprintf(client_fifo, sizeof(client_fifo), "./tmp/w_%s", pid_name);
    unlink(client_fifo);

    snprintf(client_fifo, sizeof(client_fifo), "./tmp/r_%s", pid_name);
    unlink(client_fifo);

    _exit(0);
}

// Signal handler for unexpected Ctrl + C (SIGINT)
void handle_signalint() {
    printf("\n\nUNEXPECTED TERMINATION\n\n");

    char client_fifo[256];
    int pid = getpid();
    char pid_name[256];
    itoa(pid, pid_name);

    snprintf(client_fifo, sizeof(client_fifo), "./tmp/w_%s", pid_name);
    unlink(client_fifo);

    snprintf(client_fifo, sizeof(client_fifo), "./tmp/r_%s", pid_name);
    unlink(client_fifo);

    // Restore default signal behavior and send SIGINT to self
    signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
}

// Function to send 'status' command and read the response
int send_status(char *client_fifo) {
    printf("Getting status...\n");

    if (write(write_fifo, "status", 6) == -1) {
        perror("write status");
        _exit(1);
    }

    if ((read_fifo = open(client_fifo, O_RDONLY)) == -1) {
        perror("open read fifo");
        _exit(1);
    }

    int n;
    while ((n = read(read_fifo, buff, BUFF_SIZE)) > 0) {
        if (write(STDOUT_FILENO, buff, n) == -1) {
            perror("write stdout");
            _exit(1);
        }
    }

    return 0;
}

// Function to execute a command and read the response
int execute(char* command, int size, char *client_fifo) {
    printf("Executing...\n");

    if (write(write_fifo, command, size) == -1) {
        perror("write command");
        _exit(1);
    }

    close(write_fifo);

    if ((read_fifo = open(client_fifo, O_RDONLY)) == -1) {
        perror("open read fifo");
        _exit(1);
    }

    int n;
    while ((n = read(read_fifo, buff, BUFF_SIZE)) > 0) {
        if (write(STDOUT_FILENO, buff, n) == -1) {
            perror("write stdout");
            _exit(1);
        }
    }

    close(read_fifo);
    return 0;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handle_signalint);
    signal(SIGTERM, handle_signalterm);

    char client_fifo[256], client_fifo2[256];
    int pid = getpid();
    char pid_name[256];
    itoa(pid, pid_name);

    // Create FIFOs for client
    snprintf(client_fifo, sizeof(client_fifo), "./tmp/w_%s", pid_name);
    if (mkfifo(client_fifo, 0666) == -1) {
        perror("create write fifo");
        _exit(1);
    }

    snprintf(client_fifo2, sizeof(client_fifo2), "./tmp/r_%s", pid_name);
    if (mkfifo(client_fifo2, 0666) == -1) {
        perror("create read fifo");
        _exit(1);
    }

    // Open server FIFO for writing
    if ((server_fifo = open("./tmp/server_fifo", O_WRONLY)) == -1) {
        perror("open server fifo");
        _exit(1);
    }

    // Send client's PID to the server
    if (write(server_fifo, &pid, sizeof(int)) == -1) {
        perror("write pid");
        _exit(1);
    }
    close(server_fifo);

    // Open client FIFOs for communication
    if ((write_fifo = open(client_fifo, O_WRONLY)) == -1) {
        perror("open write fifo");
        _exit(1);
    }

    bool execute_mode = (strcmp(argv[1], "execute") == 0);

    if (execute_mode) {
        // Execute mode
        char *mode = argv[3];

        if (!(strcmp(mode, "-u") == 0 || strcmp(mode, "-p") == 0)) {
            perror("invalid arguments");
            _exit(1);
        }

        // Create command string
        int total_size = 0;
        for (int i = 1; i < argc; i++) {
            total_size += strlen(argv[i]) + 1;
        }

        char *args_string = malloc(total_size);
        if (!args_string) {
            perror("malloc");
            _exit(1);
        }

        strcpy(args_string, argv[1]);
        for (int i = 2; i < argc; i++) {
            strcat(args_string, " ");
            strcat(args_string, argv[i]);
        }

        printf("String: %s\n", args_string);
        execute(args_string, total_size, client_fifo2);
        free(args_string);

    } else {
        // Status mode
        send_status(client_fifo2);
    }

    // Cleanup: remove FIFOs
    unlink(client_fifo);
    unlink(client_fifo2);

    return 0;
}
