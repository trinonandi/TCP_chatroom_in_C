# TCP Chatroom
A simple TCP Chatroom created using socket programming in C
### Inspiration
-------------
I was inspired by the practical problems that I had solved in Computer Network lab as a part of my 6th semester B-Tech curriculum. In this lab, I had learned about the fundamentals of socket programming and created some basic client server programs using C language. But, I wanted to take my learning to the next level and tried to implement a fully functional Chat System using TCP client server architecture.

### Behind The Screen
-------
Though the application may look pretty simple at a first glance, lot of stuffs are happening behind the screen.
There are just two C files: The client.c and the server.c. 

***Let me explain the logic behind the server.c first:***

- The server will accept the client requests and setup a connection with the client. The client details will be then stored in a user defined structure named `client_t`. The client details will include the `sockaddr_in` type variable, holding details about all the client address, a socket file descriptor, a unique user id that will be generated for each client request and the client username that will be received once the request is accepted.
- I have created an array of pointers to client_t named `clients[]` in order to track the current clients in the system. This array will hold the structure variable of all the clients present and once a client request is accepted, the client structure will be added to this array.
- After that, the server will start a thread dedicated to that client. That is, each client will be handled in a separate thread. The thread will run the `manage_client()` function. This function is responsible for receiving messages from the client, then logging them to the sever console and sending them to all the other clients except the sender. The details of the available clients can be fetched from `clients[]`.
- Once the client sends a message `"exit"`or hits `ctrl + c`, the server will remove that client from the `clients[]` , send a left message to all other clients as well as log it and finally, the connection will be closed.
- The `clients[]`is actually a critical section as it can be accessed simultaneously for a large number of client requests. For this, I had created two synchronised methods `add_client()` and `remove_client()` that will add or remove a client structure variable from `clients[]`.

***Now the logic behind the client.c :***

- It is comparatively simple than the server. The client will send a connect request to the server and after the request is accepted, a prompt will appear asking the user name. After entering a valid user name, the client will get access to the chat room.
- There are two threads for each client. One thread is to handle the incoming messages from other clients via the server and another thread is to send message to server.
- There are two functions `receive_msg_handler()` and `send_msg_handler()` that will run on the respective threads. If the user sends `"exit"` or hits `ctrl+c`, the client application will quit. 

### How to Run
----
>***Note:*** A Linux based system is required to run the application

1. Compile the server.c by executing the following code in terminal `gcc -Wall -g3 -fsanitize=address -pthread server.c -o server`
2. Run the server as `./server 8888` or any port number you want as command line argument
3. Open a new terminal and run the client.c as `gcc -Wall -g3 -fsanitize=address -pthread client.c -o client`
4. Run the client as `./client 8888` or any port number in which the server is hosted

>***Note:*** The client port number must match the server port number. The server must be running before executing the client.c

### Working Demo
-----
<img src ="https://github.com/trinonandi/TCP_chatroom_in_C/blob/main/demo.gif" alt="demo.gif" width="1920" height="1080" />
