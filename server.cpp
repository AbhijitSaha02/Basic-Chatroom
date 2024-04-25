#include <sys/types.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <errno.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#define MAX_LEN 300
#define NUM_COLORS 10

using namespace std;

struct terminal
{
    int id;
    string name;
    int socket;
    thread thr;
};

vector<terminal> clients;
string reset_attr = "\033[0m";
string colors[NUM_COLORS] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m", "\033[37m", "\033[91m", "\033[92m", "\033[93m"};
int seed = 0;
mutex cout_mutex, clients_mutex;

string color(int code)
{
    return colors[code % NUM_COLORS];
}

// Setting the name of the client
void set_name(int id, char name[])
{
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id == id)
        {
            clients[i].name = string(name);
        }
    }
}

// To handle the synchronization of the print statements
void shared_print(string str, bool endLine = true)
{
    lock_guard<mutex> guard(cout_mutex);
    cout << str;
    if (endLine)
    {
        cout << endl;
    }
}

// Broadcast a message to all clients except the sender
int broadcast_message(string message, int sender_id)
{
    char temp[MAX_LEN];
    strcpy(temp, message.c_str());

    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id != sender_id)
        {
            send(clients[i].socket, temp, sizeof(temp), 0);
        }
    }
}

// Broadcast a number to all the clients except the sender
int broadcast_message(int num, int sender_id)
{
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id != sender_id)
        {
            send(clients[i].socket, (char *)&num, sizeof(num), 0);
        }
    }
}

void end_connection(int id)
{
    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].id == id)
        {
            lock_guard<mutex> guard(clients_mutex);
            clients[i].thr.detach();
            clients.erase(clients.begin() + i);
            closesocket(clients[i].socket);
            return;
        }
    }
}

void handle_client(int client_socket, int id)
{
    char name[MAX_LEN], str[MAX_LEN];
    recv(client_socket, name, sizeof(name), 0);
    set_name(id, name);

    string welcome_message = string(name) + "has joined";
    broadcast_message("#NULL", id);
    broadcast_message(id, id);
    broadcast_message(welcome_message, id);
    shared_print(color(id) + welcome_message + reset_attr);

    while (1)
    {
        int bytes_recieved = recv(client_socket, str, sizeof(str), 0);
        if (bytes_recieved <= 0)
        {
            return;
        }

        if (strcmp(str, "#exit") == 0)
        {
            string leaving_message = string(name) + "has left";
            broadcast_message("#NULL", id);
            broadcast_message(id, id);
            broadcast_message(leaving_message, id);
            shared_print(color(id) + leaving_message + reset_attr);
            end_connection(id);
            return;
        }

        broadcast_message(string(name), id);
        broadcast_message(id, id);
        broadcast_message(string(str), id);
        shared_print(color(id) + name + " : " + reset_attr + str);
    }
}

int main()
{
    int server_socket;
    if (server_socket = socket(AF_INET, SOCK_STREAM, 0) == -1)
    {
        perror("Socket: ");
        exit(-1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(10000);
    server.sin_addr.s_addr = INADDR_ANY;
    memset(&(server.sin_zero), 0, sizeof(server.sin_zero));

    if ((bind(server_socket, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) == -1)
    {
        perror("Bind error: ");
        exit(-1);
    }

    if ((listen(server_socket, 8)) == -1)
    {
        perror("Listen error: ");
        exit(-1);
    }

    struct sockaddr_in client;
    int client_socket;
    int len = sizeof(sockaddr_in);

    cout << colors[NUM_COLORS - 1] << "\n\t ===== Welcome to the chat-room =====" << endl
         << reset_attr;

    while (1)
    {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client, &len)) == -1)
        {
            perror("Accept error: ");
            exit(-1);
        }

        seed++;
        thread t(handle_client, client_socket, seed);
        lock_guard<mutex> guard(clients_mutex);
        clients.push_back({seed, string("Anonymous"), client_socket, (move(t))});
    }

    for (int i = 0; i < clients.size(); i++)
    {
        if (clients[i].thr.joinable())
        {
            clients[i].thr.join();
        }
    }

    closesocket(server_socket);
    return 0;
}