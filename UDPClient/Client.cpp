#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <sstream>
#include <string>
#include <Windows.h>
using namespace std;

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 4096
#define SERVER_IP "127.0.0.1"
#define DEFAULT_PORT "5555"

SOCKET client_socket;

DWORD WINAPI Sender(void* param)
{
    while (true) {
        char query[DEFAULT_BUFLEN - 20];
        cin.getline(query, DEFAULT_BUFLEN - 20);
        string fullMessage = query;
        send(client_socket, fullMessage.c_str(),
            fullMessage.length(), 0);
    }
}

DWORD WINAPI Receiver(void* param)
{
    while (true) {
        char response[DEFAULT_BUFLEN];
        int result = recv(client_socket, response, DEFAULT_BUFLEN, 0);
        response[result] = '\0';
        istringstream iss(response);
        string msg;
        getline(iss, msg);
        cout << msg << endl;
    }
}

BOOL ExitHandler(DWORD whatHappening)
{
    switch (whatHappening)
    {
    case CTRL_C_EVENT: // closing console by ctrl + c
    case CTRL_BREAK_EVENT: // ctrl + break
    case CTRL_CLOSE_EVENT: // closing the console window by X button
        return(TRUE);
        break;
    default:
        return FALSE;
    }
}

int main()
{
    system("title McDonald's Приложение");
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup завершился ошибкой: %d\n", iResult);
        return 1;
    }
    addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    iResult = getaddrinfo(SERVER_IP, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo завершился ошибкой: %d\n", iResult);
        WSACleanup();
        return 2;
    }

    addrinfo* ptr = nullptr;
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        client_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (client_socket == INVALID_SOCKET) {
            printf("Сокет завершился ошибкой: %ld\n", WSAGetLastError());
            WSACleanup();
            return 3;
        }
        iResult = connect(client_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(client_socket);
            client_socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (client_socket == INVALID_SOCKET) {
        printf("Не удалось подключиться к серверу!\n");
        WSACleanup();
        return 5;
    }

    CreateThread(0, 0, Sender, 0, 0, 0);
    CreateThread(0, 0, Receiver, 0, 0, 0);

    Sleep(INFINITE);
}
