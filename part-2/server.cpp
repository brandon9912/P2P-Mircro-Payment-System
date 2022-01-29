#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <pthread.h>
#include <vector>

using namespace std;

int online_user_num = 0;

struct user{
    string name = "none";
    int money = 0;
    string IP = "none";
    int port = 0;
    int online = 0;
    int socket_number = 0;
};

vector<user> users;

void *connection_handler(void *);
string reg(string msg);
string login(string msg);
string list(string username);
string trans(string msg);

int main(int argc, char *argv[])
{
    // Create a socket
    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if(listening == -1)
    {
        cerr << "Socket failed to create\n";
        return -1;
    }
    // Bind the socket to a IP/PORT
    struct sockaddr_in server;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[1]));

    if( bind(listening,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        puts("bind failed");
    }
	puts("bind done.");
    // Mark the socket for listening in
    listen(listening, 3);
    // Accept connections
    int new_socket, *new_sock, c;
    c = sizeof(struct sockaddr_in);
    struct sockaddr_in client;
    puts("Waiting for connections.");
    while( (new_socket = accept(listening, (struct sockaddr *)&client, (socklen_t*)&c)) ){
		puts("Connection accepted.");


		pthread_t sniffer_thread;
		new_sock = (int*)malloc(1);
		*new_sock = new_socket;
		
		if( pthread_create( &sniffer_thread, NULL,  connection_handler, (void*) new_sock) < 0){
			perror("could not create thread");
			return 1;
		}
		
		puts("Handler assigned");
	}
	
	if (new_socket < 0){
		perror("accept failed");
		return 1;
	}
	
	return 0;
}

void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size; 
    char *message, client_message[2000];
    string username = "No one has logged in yet";

    //Receive a message from client
	while( (read_size = recv(sock, client_message, 2000, 0)) > 0 )
    {
        puts("======================================");
        cout << username << " ";
        puts("client msg:");
        puts(client_message);
        puts("======================================");

        //client command 
        string msg = client_message;
        string msg2client;
        if(msg.substr(0,3) == "REG")
        {
            msg2client = reg(msg);
            write(sock, msg2client.c_str(), msg2client.length());
        }
        else if(msg.substr(0,3) == "Lis")
        {
            msg2client = list(username);
            write(sock, msg2client.c_str(), msg2client.length());
        }
        else if(msg.substr(0,3) == "Exi"){
            //Free the socket pointer
            online_user_num--;
            for(int i = 0; i < users.size(); i++){
                if(users[i].name == username){
                    users[i].online = 0;
                }
            }
            msg2client = "Thank you for using P2P-Micro-Payment-System.";
            write(sock, msg2client.c_str(), msg2client.length());
	        free(socket_desc);
	        return 0;
        }
        else//login or trans
        {
            int pound = msg.find('#');
            string payer = msg.substr(0, pound);
            int pound2 = msg.find('#', pound + 1);
            string payee = msg.substr(pound2 + 1);

            if(pound2 != string::npos)//trans
            {
                int sock_fd;
                for(int i = 0; i < users.size(); i++)
                {
                    if(users[i].name == payer)
                    {
                        sock_fd = users[i].socket_number;
                    }
                }
                int sock_payee;
                for(int i = 0; i < users.size(); i++)
                {
                    if(users[i].name == payee){
                        sock_payee = users[i].socket_number;
                    }
                }
                if(sock != sock_payee)//msg 不是payee傳來的
                {
                    msg2client = "u have to send the msg to the right payee first!\n";
                    write(sock_fd, msg2client.c_str(), msg2client.length());
                }
                else
                {
                    msg2client = trans(msg);
                    write(sock_fd, msg2client.c_str(), msg2client.length());
                }
            }
            else//login
            {
                username = msg.substr(0, pound);
                for(int i = 0; i < users.size(); i++)
                {
                    if(users[i].name == username)
                    {
                        users[i].socket_number = sock;
                    }
                }
                msg2client = login(msg);
                write(sock, msg2client.c_str(), msg2client.length());
            }
        }
	}
}

string reg(string msg) // msg: REGISTER#<user name>
{
    string username;
    int start = msg.find('#');
    username = msg.substr(start + 1);

    for(int i = 0; i < users.size(); i++)
    {
        if(users[i].name == username)
        {
            string msg = "Error: User already exist. Please try another user name.\n";
            return msg;
        }
    }
 
    user newuser;
    newuser.name = username;
    newuser.money = 10000;
    newuser.IP = "127.0.0.1";
    users.push_back(newuser);
    string msg2client = "User register succeeded.\n";
    return msg2client;
}

string login(string msg)//msg: <user name>#<port number>
{
    if(online_user_num == 3){
        string msg2client = "Online user exceed maximum capacity. Please try again later.\n";
        return msg2client;
    }

    int pound = msg.find("#");
    string username = msg.substr(0, pound);
    int port_number = stoi(msg.substr(pound + 1), nullptr, 10);
    if((port_number > 65535)||(port_number < 1024)){//port number 超出範圍
        string msg2client = "Error: port number needs to be between 1024 and 65535\n";
        return msg2client;
    }
    int find = -1;
    for(int i = 0; i < users.size(); i++){
        if(users[i].name == username){
            if(users[i].online == 1){
                string msg2client = "Error: User already logged in\n";
                return msg2client;
            }
            users[i].port = port_number;
            users[i].online = 1;
            online_user_num++;
            find = i;
        }
    }
    if(find == -1){//還沒註冊
        string msg2client = "not register yet. try again!\n";
        return msg2client;
    }
    
    string msg2client;
    msg2client.append(to_string(users[find].money));
    msg2client.append("\n");
    msg2client.append(to_string(online_user_num));
    msg2client.append("\n");
    for(int i = 0; i < users.size(); i++)//find all online users
    {
        if(users[i].online == 1){
            msg2client.append(users[i].name + "#");
            msg2client.append(users[i].IP + "#");
            msg2client.append(to_string(users[i].port)+"\n");
        }
    }
    return msg2client;
}

string list(string username)
{
    int find = -1;
    string msg2client;
    for(int i = 0; i < users.size(); i++){
        if(users[i].name == username && users[i].online == 1){
            find = i;
        }
    }
    if(find == -1){
        msg2client = "Error: No one in online\n";
        return msg2client;
    }
    msg2client.append(to_string(users[find].money));
    msg2client.append("\n");
    msg2client.append( to_string(online_user_num));
    msg2client.append("\n");
    for(int i = 0; i < users.size(); i++){
        if(users[i].online == 1)//find all online users
        {
            msg2client.append(users[i].name + "#");
            msg2client.append(users[i].IP + "#");
            msg2client.append(to_string(users[i].port)+"\n");
        }
    }
    return msg2client;
}

string trans(string msg)//msg: <payer>#<amount>#<payee>
{
    int pound1 = msg.find('#');
    int pound2 = msg.find('#', pound1 + 1);
    string payer = msg.substr(0,pound1);
    int amount = stoi(msg.substr(pound1 + 1, pound2 - pound1 - 1), nullptr, 10 );
    string payee = msg.substr(pound2+1);
    int payer_find = -1;
    int payee_find = -1;
    for(int i = 0; i < users.size(); i++)
    {
        if(users[i].name == payer)
        {
            payer_find = i;
        }
    }
    for(int i = 0; i < users.size(); i++)
    {
        if(users[i].name == payee)
        {
            payee_find = i;
        }
    }

    string msg2client = "transfer OK!\n";
    //error handling
    if(payer_find == payee_find)
    {
        msg2client = "Error: Transfering to yourself\n";
        return msg2client;
    }
    if(payer_find == -1)
    {    
        msg2client = "Error: User(Payer) not found\n";
        return msg2client;
    }
    if(payee_find == -1)
    {
        msg2client = "Error: User(Payee) not found\n";
        return msg2client;
    }
    if(users[payer_find].online == 0)
    {
        msg2client = "Error: User(Payer) not online\n";
        return msg2client;
    }
    if(users[payee_find].online == 0){
        msg2client = "Error: User(Payee) not online\n";
        return msg2client;
    }
    if(users[payer_find].money < amount)
    {
        msg2client = "Error: User balance insufficient\n";
        return msg2client;
    }

    users[payer_find].money -= amount;
    users[payee_find].money += amount;
    return msg2client;
}