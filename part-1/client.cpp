#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>	//inet_addr
#include <netinet/in.h>
#include <pthread.h>

using namespace std;

void command_page();
void *receive_thread(void *sd);
void receiving(int server_sd);
void sending();
int sd;
char server_reply[2000];

int main(int argc, char *argv[])
{
    //socket creation and connection
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd == -1){
        printf("Socket failed to create\n");
    }
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));
    if(connect(sd, (struct sockaddr *)&server, sizeof(server)) < 0){
        printf("Connection error.\n");
        return 1;
    }
    else{
        printf("Connected to server.\n");
    }

    int sd_t = socket(AF_INET, SOCK_STREAM, 0);
    if(sd_t == -1){
        printf("Socket failed to create\n");
    }

    printf("Please enter client port number:\n");
    char port[10];
    scanf("%s", port);

    struct sockaddr_in client_;
    socklen_t addrlen = sizeof(client_);
    client_.sin_addr.s_addr = INADDR_ANY;
    client_.sin_family = PF_INET;
    client_.sin_port = htons(atoi((char *) port));
    bind(sd_t, (struct sockaddr *)&client_, sizeof(client_));
    listen(sd_t, 5);

    pthread_t tid;
    pthread_attr_t attr;
   	pthread_attr_init(&attr);
    pthread_create(&tid, &attr, &receive_thread, &sd_t);

    string list;//record username from server
    int command;

    command_page();
    while(1)
    {
        scanf("%d", &command);
        if(command == 0)//register
        {
            printf("Please enter user name:\n");
            char user[20];
            scanf("%s", user);
            string msg;
            msg.append("REGISTER#");
            msg.append(user);
            send(sd,msg.c_str(),sizeof(msg),0);
            printf("Waiting for server to respond...\n");
            if(recv(sd, server_reply , 2000 , 0) < 0){
		        puts("recv failed");
	        }
            else{
                puts(server_reply);
                printf("Register Succeeded!\n");
            }
            command_page();
            bzero(server_reply, 2000);
        }
        else if(command == 1) //login
        {
            printf("Please enter user name:\n");
            char user[20];
            scanf("%s", user);
            string msg;
            msg.append(user);
            msg.append("#");
            msg.append(port);
            send(sd,msg.c_str(),sizeof(msg),0);
            printf("Waiting for server to respond...\n");
            if(recv(sd, server_reply, 2000 , 0) < 0){
		        puts("recv failed");
	        }
            else{
                puts(server_reply);
            }
            command_page();
            bzero(server_reply, 2000);

        }
        else if(command == 2)//List
        {
            string msg = "List";
            send(sd,msg.c_str(),sizeof(msg),0);
            printf("Waiting for server to respond...\n");
            if(recv(sd, server_reply, 2000 , 0) < 0){
		        puts("recv failed");
	        }
            else{
                puts(server_reply);
                printf("List:");
            }
            command_page();
            bzero(server_reply, 2000);
        }
        else if(command == 3)//transaction
        {
            sending();
            if(recv(sd, server_reply, 2000 , 0) < 0){
		        puts("recv failed");
	        }
            else{
                puts(server_reply);
                printf("Transfer Succeeded!\n");
            }
            command_page();
            bzero(server_reply, 2000);
        }
        else if(command == 4){//exit
            string msg = "Exit";
            send(sd, msg.c_str(), sizeof(msg), 0);
            if(recv(sd, server_reply, 2000 , 0) < 0){
		        puts("recv failed");
	        }
            else{
                puts(server_reply);
            }
            bzero(server_reply, 2000);
            close(sd);
            break;
        }
        else {
            printf("Invalid command!\n");
            command_page();
        }
    };
    return 0;
}

void command_page() {
    printf("======================================\n");
    printf("Please enter command: \n");
    printf("0: Register\n");
    printf("1: Login\n");
    printf("2: List\n");
    printf("3: Transaction\n");
    printf("4: Exit\n");
    printf("======================================\n");
    printf("> ");
}

void *receive_thread(void *server_fd)
{
    int s_fd = *((int *)server_fd);
    while (1)
    {
        sleep(2);
        receiving(s_fd);
    }
}

void receiving(int server_fd)
{
    char clientmsg[2000];//record client message
    int client_sd, valread;
    struct sockaddr_in client_;
    socklen_t addrlen = sizeof(client_);
    client_sd = accept(server_fd, (struct sockaddr*) &client_, &addrlen);
    
    //receiving message from client
    valread = recv(client_sd, clientmsg, 2000 , 0);

    //sending to server
    send(sd,clientmsg,sizeof(clientmsg),0);
    bzero(clientmsg, 2000);
    close(client_sd);
}

void sending()
{
    bzero(server_reply, 2000);
    send(sd,"List",sizeof("List"),0);
    recv(sd, server_reply, 2000 , 0);
    string list;//record username from server
    list = server_reply;
    bzero(server_reply, 2000);

    string user;
    printf("Please enter your user name:");
    cin >> user;

    string payee;
    printf("Please enter recepient user name:");
    cin >> payee;
    int n = list.find(payee);
    int pound2 = list.find("#", n + payee.length() + 1);
    string payee_ip = list.substr(n + payee.length() + 1, pound2 - (n + payee.length() + 1));
    string payee_port = list.substr(pound2 + 1, 4);
    
    string amount;
    printf("Please enter payment amount:");
    cin >> amount;
    string msg = user;
    msg.append("#");
    msg.append(amount);
    msg.append("#");
    msg.append(payee);
    char *payee_port_ = const_cast<char*>(payee_port.c_str());

    int client_sd;
    char client_reply[2000];

    client_sd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_sd == -1){
        printf("Socket failed to create\n");
    }
    struct sockaddr_in client_send;
    client_send.sin_addr.s_addr = INADDR_ANY;
    client_send.sin_family = AF_INET;
    client_send.sin_port = htons(stoi(payee_port, nullptr, 10));
    if(connect(client_sd, (struct sockaddr *)&client_send, sizeof(client_send)) < 0){
        printf("Connection error!\n");
    }
    else{
        printf("Connected to server.\n");
    }

    send(client_sd,msg.c_str(),sizeof(msg),0);

    close(client_sd);
}