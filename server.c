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

// Client Structure
// Every client will have an unique uid, 
// a name, an address description struct 
// and a socket file descriptor
typedef struct {
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[NAME_LEN];
} client_t;


// global variables
static _Atomic unsigned int client_count = 0;
static int uid = 10;
client_t *clients[MAX_CLIENT];
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;


// global function prototypes
void print_ip_addr(struct sockaddr_in);
void str_overwrite_stdout(void);
void str_trim_lf(char*, int);
void add_client(client_t* );
void remove_client(int);
void send_message(char* , int);
void* manage_client(void* );


// main function
int main(int argc, char **argv){

    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // setting IP and Port number
    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);

    // setting socket and thread variables
    int listenfd = 0, connfd = 0;
    struct sockaddr_in server_addr, client_addr;
    pthread_t tid;

    // Creating server socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // Server address setting
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // Signals
    signal(SIGPIPE, SIG_IGN);
    int option = 1;
    if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
		perror("ERROR: setsockopt failed");
    return EXIT_FAILURE;
	}


    // Bind
    if(bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("ERROR: binding failed\n");
        return EXIT_FAILURE;
    }

    // Listen
    if(listen(listenfd, 10) < 0){
        printf("ERROR: listen error\n");
        return EXIT_FAILURE;
    }

    printf("<==== CHATROOM SERVER LOG ====>\n");

    int run = 1;
    while(run){
        socklen_t client_size = sizeof(client_addr);
        connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_size);

        // check for MAX_CLIENT
        if(client_count + 1 == MAX_CLIENT){
            printf("Maximum clients connected\n");
            printf("Rejected IP : ");
            print_ip_addr(client_addr); // user defined function to print the rejected ip
            close(connfd);
            continue;
        }

        // client settings
        client_t *client = (client_t *)malloc(sizeof(client_t));
        client->address = client_addr;
        client->sockfd = connfd;
        client->uid = uid++;
        
        // Adding client to clients[]
        add_client(client);
        pthread_create(&tid, NULL, &manage_client, (void*)client);

        // Reduce CPU usage
        sleep(1);

    }

    return EXIT_SUCCESS;
}


// global function definitions

void print_ip_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d \n", 
            addr.sin_addr.s_addr & 0xff,
            (addr.sin_addr.s_addr & 0xff00) >> 8,
            (addr.sin_addr.s_addr & 0xff0000) >> 16,
            (addr.sin_addr.s_addr & 0xff000000) >> 24
        );
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

// functions to add client in the clients[]
// synchronized function
void add_client(client_t *client){
    pthread_mutex_lock(&client_mutex);
    for(int i=0; i<MAX_CLIENT; i++){
        if(!clients[i]){
            clients[i] = client;
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

// function to remove a client from the clients[]
// we will recognize a client by its uid as it is unique
// synchronized function
void remove_client(int uid){
    pthread_mutex_lock(&client_mutex); 
    for(int i=0; i<MAX_CLIENT; i++){
        if(clients[i] != NULL && clients[i]->uid == uid ){
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

// function to send message to all the clients except the sender
// synchronized function
void send_message(char* message, int uid){
    pthread_mutex_lock(&client_mutex); 
    for(int i=0;i<MAX_CLIENT; i++){
        if(clients[i] != NULL && clients[i]->uid != uid){
            int result = write(clients[i]->sockfd, message, strlen(message));
            if(result < 0){
                printf("ERROR: failed to send message\n");
                break;
            }
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

// primary function to manage the clients
void *manage_client(void *arg){
    char buffer[BUFFER_SIZE];
    char name[NAME_LEN];
    int leave_flag = 0; // 1 means a client has left the room
    client_count++;

    client_t *client = (client_t*)arg;
    
    // setting the client name from client side
    if(recv(client->sockfd, name, NAME_LEN, 0) <= 0 || strlen(name) <  2 || strlen(name) >= NAME_LEN-1){
		printf("Didn't enter the name.\n");
		leave_flag = 1;
	} else{
		strcpy(client->name, name);
		sprintf(buffer, "%s has joined the room\n", client->name);
		printf("%s", buffer);
		send_message(buffer, client->uid);
	}

    memset(buffer,'\0',BUFFER_SIZE);    // flushing the buffer

    while(leave_flag != 1){ // if leaf_flag == 1, then it means that client has left the room. so break
        int receive_result = recv(client->sockfd, buffer, BUFFER_SIZE, 0);
        if(receive_result >= 0){
            if(strlen(buffer) > 0){
                // means that the message received is not an empty message
                // then send the message to all the clients
                send_message(buffer, client->uid);
                str_trim_lf(buffer, strlen(buffer));

                // printing the message log in server console
                printf("%s\n", buffer);
            }
            else if(receive_result == 0 || strcmp(buffer, "exit") == 0){
                // means that a client is leaving the room
                sprintf(buffer, "%s has left the room\n", client->name);
                printf("%s", buffer);
                send_message(buffer, client->uid);
                leave_flag = 1;
            }
            else{
                // Error occured
                printf("ERROR: failed to receive data\n");
                leave_flag = 1;
            }
        }

        memset(buffer,'\0',BUFFER_SIZE);    // flushing the buffer
    }
    close(client->sockfd);  // closing the socket connection
    remove_client(client->uid);     // removing client from clients[]
    free(client);   // deallocating memory occupied by client varible
    client_count--; // decreasing client count
    pthread_detach(pthread_self()); // detatch from the client manage thread
    return NULL;
}