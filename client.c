#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENT 100
#define BUFFER_SIZE 2048
#define NAME_LEN 32

// global variables
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[NAME_LEN];

// global function prototypes
void str_overwrite_stdout(void);
void str_trim_lf(char* , int);
void catch_ctrl_c_and_exit(int);
void send_msg_handler(void);
void receive_msg_handler(void);


int main(int argc, char **argv){

    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // setting IP and Port number
    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);

    signal(SIGINT, catch_ctrl_c_and_exit);

    printf("Enter your name: ");
    fgets(name, NAME_LEN, stdin);
    str_trim_lf(name, strlen(name));
    
    if(strlen(name) > NAME_LEN - 1 || strlen(name) < 2){
        printf("INVALID NAME: name must be 3 to 32 characters long\n");
        return EXIT_FAILURE;
    }

    // setting socket
    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // Connect
    int err = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(err == -1){
        printf("ERROR: Client cannot connect to server\n");
        return EXIT_FAILURE;
    }

    // Client name setup
    send(sockfd, name, NAME_LEN, 0);
    printf("<==== WELCOME TO THE CHATROOM ====>\n");

    // setting the sender threads
    pthread_t sender_thread;
    if(pthread_create(&sender_thread, NULL, (void*)send_msg_handler, NULL) != 0){
        printf("ERROR: failed to create sender thread\n");
        return EXIT_FAILURE;
    }

    // setting the receiver thread
    pthread_t receiver_thread;
    if(pthread_create(&receiver_thread, NULL, (void*)receive_msg_handler, NULL) != 0){
        printf("ERROR: failed to create receiver thread\n");
        return EXIT_FAILURE;
    }


    while(1){
        if(flag){
            printf("Good Bye\n");
            break;
        }
    }

    close(sockfd);
    return EXIT_SUCCESS;
}

void str_overwrite_stdout(){
    printf("\r%s", "> ");
    fflush(stdout);
}

void str_trim_lf(char* arr, int length){
    for(int i=0; i<length; i++){
        if(arr[i] == '\n'){
            arr[i] = '\0';
            break;
        }
    }
}

void catch_ctrl_c_and_exit(int sig){
    flag = 1;
}

// it will handle all the messages received from the server
void receive_msg_handler(){
    char message[BUFFER_SIZE] = {};
    while(1){
        int result = recv(sockfd, message, BUFFER_SIZE, 0);
        if(result > 0){
            // successfully received
            printf("%s", message);
            str_overwrite_stdout();
        }
        else if(result == 0){
            // Error occured while receiving message
            printf("ERROR: failed to receive messages\n");
            break;
        }

        // resetting the buffer
        memset(message, '\0', sizeof(message));
    }
}

// it will send messages to the server
void send_msg_handler(){
    char message[BUFFER_SIZE] = {};
	char buffer[BUFFER_SIZE + 32] = {};

  while(1) {
  	str_overwrite_stdout();
    fgets(message, BUFFER_SIZE, stdin);
    str_trim_lf(message, BUFFER_SIZE);

    if (strcmp(message, "exit") == 0) {
			break;
    } else {
      sprintf(buffer, "%s: %s\n", name, message);
      send(sockfd, buffer, strlen(buffer), 0);
    }

		bzero(message, BUFFER_SIZE);
    bzero(buffer, BUFFER_SIZE+ 32);
  }
  catch_ctrl_c_and_exit(2);
}