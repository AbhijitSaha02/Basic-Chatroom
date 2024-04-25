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
#include <csignal>
#define MAX_LEN 300
#define NUM_COLORS 10

using namespace std;

bool exit_flag = false;
thread t_send, t_recv;
int client_socket;
string reset_attr = "\033[0m";
string colors[NUM_COLORS] = {"\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m", "\033[37m", "\033[91m", "\033[92m", "\033[93m"};

// Handler for "Ctrl + C"
void catch_ctrl_c(int signal)
{
    char str[MAX_LEN] = "#exit";
    send(client_socket, str, sizeof(str), 0);
    exit_flag = true;
    t_send.detach();
    t_recv.detach();
    closesocket(client_socket);
    exit(signal);
}

string color(int code)
{
    return colors[code % NUM_COLORS];
}

// Erasing text from terminal
int eraseText(int count)
{
    char back_space = 8;
    for (int i = 0; i < count; i++)
    {
        cout << back_space;
    }
}

// Send message to everyone
void send_message(int client_socket)
{
    while (1)
    {
        cout << colors[1] << "You : " << reset_attr;
        char str[MAX_LEN];
        cin.getline(str, MAX_LEN);
        send(client_socket, str, sizeof(str), 0);

        if (strcmp(str, "#exit") == 0)
        {
            exit_flag = true;
            t_recv.detach();
            closesocket(client_socket);
            return;
        }
    }
}

// Recieve messages
void recv_message(int client_socket)
{
    while (1)
    {
        if (exit_flag)
        {
            return;
        }

        char name[MAX_LEN], str[MAX_LEN];
        int color_code;
        int bytes_recieved = recv(client_socket, str, sizeof(name), 0);
        if (bytes_recieved <= 0)
        {
            continue;
        }

        recv(client_socket, (char *)&color_code, sizeof(color_code), 0);
        recv(client_socket, str, sizeof(str), 0);
        eraseText(6);

        if (strcmp(name, "#NULL") != 0)
        {
            cout << color(color_code) << name << ": " << reset_attr << endl;
        }
        else
        {
            cout << color(color_code) << str << endl;
        }

        cout << colors[1] << "You :" << reset_attr;
        fflush(stdout);
    }
}

int main()
{
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket: ");
        exit(-1);
    }

    struct sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(10000);
    client.sin_addr.s_addr = INADDR_ANY;
    memset(&client.sin_zero, 0, sizeof(client.sin_zero));

    if((connect(client_socket, (struct sockaddr *) &client, sizeof(struct sockaddr_in))) == -1) {
        perror("Connect: ");
        exit(-1);
    }

    signal(SIGINT, catch_ctrl_c);
    char name[MAX_LEN];
    cout << "Enter your name: ";
    cin.getline(name, MAX_LEN);
    send(client_socket, name, sizeof(name), 0);

    cout << colors[NUM_COLORS - 1] << "\n\t  ====== Welcome to the chat-room ======" << endl << reset_attr;

    thread t1(send_message, client_socket);
    thread t2(recv_message, client_socket);

    t_send = move(t1);
    t_recv = move(t2);

    if(t_send.joinable()) {
        t_send.join();
    }
    if(t_recv.joinable()) {
        t_recv.join();
    }

    return 0;
}