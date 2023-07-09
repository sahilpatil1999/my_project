#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
#include<unistd.h>
#include<pthread.h>

struct AcceptSocket{
    int acceptSocketFD;
    struct sockaddr_in address;
};

struct ConnectSocket{
    int id;
    int clientSocketFD;
    struct sockaddr_in address;
    struct ConnectSocket *next;
};

#define serverPort 1223

//Function Declaration
struct sockaddr_in *createIPv4Address(const char *, int);
struct AcceptSocket *acceptIncomingConnection(int serverSocketFD);
void *receiveAndPrint(void *socketFD);
void startAcceptingIncomingConnections(int serverSocketFD);
void *sender(void *fd);

void server(void *ptr){
    int serverSocketFD = socket(AF_INET, SOCK_STREAM, 0); //(Address Family I NET -> IPV4),(asking for TCP socket),(layer) it will return socket file descriptor 

    struct sockaddr_in *serverAddress = createIPv4Address("127.0.0.1", serverPort);

    int result = bind(serverSocketFD, (struct sockaddr *)serverAddress, sizeof(*serverAddress));
    printf("result: %d\n", result);
    if(result == -1){
        perror("Socket bind Fail:");
        exit(1);
    }

    int listenResult = listen(serverSocketFD, 10); //10 -> number of connections

    startAcceptingIncomingConnections(serverSocketFD);

    shutdown(serverSocketFD, SHUT_RD);
}

void client(void *ptr){
    
    sleep(1);
    printf("Press Enter to start client after allserver are ready to listen");
    getchar();
    int choice=1;
    pthread_t send_thread;
    int ports[] = {1223,1228}, i = 0;
    struct ConnectSocket *head = NULL, *t1 = NULL;

    while(i < 2){
     //   if(ports[i] != serverPort){
        int socketFD = socket(AF_INET, SOCK_STREAM, 0); //(Address Family I NET -> IPV4),(asking for TCP socket),(layer) it will return socket file descriptor 

        struct sockaddr_in *address = createIPv4Address("127.0.0.1", ports[i]);

        
        connect(socketFD, (struct sockaddr *)address, sizeof(struct sockaddr));
        
        struct ConnectSocket *temp = malloc(sizeof(struct ConnectSocket));
        temp->address = *address;
        temp->clientSocketFD = socketFD;
        temp->id = i + 1;
        temp->next = NULL;

        if(head == NULL){
            head = temp;
            t1 = head;
        }
        else{
            while(t1->next != NULL){
                t1 = t1->next;
            }
            t1->next = temp;
        }
     //   }
        i++;
    }
    while(choice){
    t1 = head;
    while(t1 != NULL){
        printf("%d : %d\n", t1->id, ntohs(t1->address.sin_port)); //ntohs coverted network bytes to port number 
        t1 = t1->next;
    }

    printf("Select Port No Option for communication or press 0 to exit client : ");
    scanf("%d", &choice);
    t1 = head;
    while(t1 != NULL){
        if(t1->id == choice){
            pthread_create(&send_thread, NULL, sender, (void *)&t1->clientSocketFD);
            pthread_join(send_thread, NULL);
            break;
        }
        t1 = t1->next;
    }
    }
}

int main(){
    pthread_t server_thread, client_thread;
    pthread_create(&server_thread, NULL, server, NULL);
    pthread_create(&client_thread, NULL, client, NULL);

    pthread_join(server_thread, NULL);
    pthread_join(client_thread, NULL);

    return 0;
}

struct sockaddr_in *createIPv4Address(const char *ip, int port){
    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
    address->sin_family = AF_INET;
    address->sin_port = htons(port); // Convert the port to network byte order

    if(strlen(ip) == 0){
        address->sin_addr.s_addr = INADDR_ANY;
    }
    else{
        inet_pton(AF_INET, ip, &(address->sin_addr));
    }

    return address;
}

struct AcceptSocket *acceptIncomingConnection(int serverSocketFD){
    struct sockaddr_in clientAddress;
    int clientAddSize = sizeof(struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD, (struct sockaddr *)&clientAddress, &clientAddSize);

    //printf("\nclientFD: %d\n", clientSocketFD);

    struct AcceptSocket *acceptSocket = malloc(sizeof(struct AcceptSocket));
    acceptSocket->address = clientAddress;
    acceptSocket->acceptSocketFD = clientSocketFD;

    return acceptSocket;
}

void *receiveAndPrint(void *socketFD){
    int clientFD = *(int *)socketFD;
    char buf[1024];
    while(true){
        ssize_t amountReceive = recv(clientFD, buf, 1024, 0);
           
        if(amountReceive > 0){
            printf("%s", buf);
            memset(buf, 0, sizeof(buf));
        }
        if(amountReceive == 0){
            break;
        }
    }
    close(clientFD);
    pthread_exit(NULL);
}

void startAcceptingIncomingConnections(int serverSocketFD){
    while(true){
        struct AcceptSocket *clientSocket = acceptIncomingConnection(serverSocketFD);

        pthread_t id;
        pthread_create(&id, NULL, receiveAndPrint, (void *)&clientSocket->acceptSocketFD);
    }
}

void *sender(void *fd){
    int clientFD = *(int *)fd;
    char buff[1024];
    printf("Enter message to exit, type 'exit':\n");
    while (1){
        fgets(buff, sizeof(buff), stdin);
        if(memcmp(buff, "exit", 4) == 0){
            break;
        }
        write(clientFD, buff, sizeof(buff));
    }
    pthread_exit(NULL);
}
