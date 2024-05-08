#include <winsock2.h>
#include <iostream>
#include <queue>
#include <string>
#include <algorithm>
using namespace std;

#define MAX_CLIENTS 30
#define DEFAULT_BUFLEN 4096
#define BURGER_TIME 5
#define SODA_TIME 1
#define FRIES_TIME 3

#pragma comment(lib, "ws2_32.lib") // Библиотека Winsock
#pragma warning(disable:4996) // Отключение предупреждения _WINSOCK_DEPRECATED_NO_WARNINGS
CRITICAL_SECTION cs;
int sec = 0;
SOCKET server_socket;
queue<string> orders;

int GetCount(string str, string find) {
    int pos = str.find(find);
    string sbstr;
    if (pos != string::npos) {
        int fpos = pos - 7 < 0 ? 0 : pos - 7;
        sbstr = str.substr(fpos, pos);
        size_t digits = sbstr.find_first_of("1234567890+-");
        if (digits <= sbstr.size()) {
            return atoi(sbstr.c_str() + digits);
        }
        else {
            return 1;
        }
    }
    else {
        return 0;
    }
}

int Counting(string str) {
    int burgers, sodas, fries, total_time;
    transform(str.begin(), str.end(), str.begin(), tolower);
    burgers = GetCount(str, "бургер");
    sodas = GetCount(str, "сода");
    fries = GetCount(str, "картофель-фри");
    total_time = burgers * BURGER_TIME + sodas * SODA_TIME + fries * FRIES_TIME;
    sec += total_time;
    return total_time;
}

void Cooking(string str) {
    int time = Counting(str);
    Sleep(time * 1000);
    sec -= time;
}

DWORD WINAPI OrderProcessing(void* param)
{
    EnterCriticalSection(&cs);
    int count = 0;
    while (true) {
        if (!orders.empty()) {
            string Order = orders.front();
            orders.pop();
            if (count == 100)
                count = 1;
            count++;
            cout << "Заказ " << count << " начат!" << endl;
            Cooking(Order);
            cout << "Заказ " << count << " завершен!" << endl;
        }
    }
    LeaveCriticalSection(&cs);
}

int main() {
    system("title Сервер McDonald's");
    InitializeCriticalSection(&cs);
    puts("Запуск сервера McDonald's... УСПЕШНО.");

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Не удалось запустить WSA. Код ошибки: %d", WSAGetLastError());
        return 1;
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Не удалось создать сокет: %d", WSAGetLastError());
        return 2;
    }

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(5555);

    if (bind(server_socket, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Ошибка привязки сокета: %d", WSAGetLastError());
        return 3;
    }

    listen(server_socket, MAX_CLIENTS);
    puts("Сервер ожидает входящих заказов...");

    fd_set readfds;
    SOCKET client_socket[MAX_CLIENTS] = {};

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            SOCKET s = client_socket[i];
            if (s > 0) {
                FD_SET(s, &readfds);
            }
        }

        if (select(0, &readfds, NULL, NULL, NULL) == SOCKET_ERROR) {
            printf("Вызов функции select завершился ошибкой: %d", WSAGetLastError());
            return 4;
        }

        SOCKET new_socket;
        sockaddr_in address;
        int addrlen = sizeof(sockaddr_in);
        if (FD_ISSET(server_socket, &readfds)) {
            if ((new_socket = accept(server_socket, (sockaddr*)&address, &addrlen)) < 0) {
                perror("Ошибка функции accept");
                return 5;
            }
            printf("Новый заказ получен, идентификатор сокета: %d, IP клиента: %s, порт клиента: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("Добавление в список сокетов по индексу %d\n", i);
                    break;
                }
            }
        }
        CreateThread(0, 0, OrderProcessing, 0, 0, 0);

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            SOCKET s = client_socket[i];
            if (FD_ISSET(s, &readfds))
            {
                getpeername(s, (sockaddr*)&address, (int*)&addrlen);
                char client_message[DEFAULT_BUFLEN];
                int client_message_length = recv(s, client_message, DEFAULT_BUFLEN, 0);
                client_message[client_message_length] = '\0';
                string check_exit = client_message;
                if (check_exit == "off")
                {
                    cout << "Клиент #" << i << " отключился\n";
                    client_socket[i] = 0;
                }
                string temp = client_message;
                temp += "\n";
                char msg[DEFAULT_BUFLEN];
                orders.push(temp);
                Counting(temp);
                for (size_t j = 0; j < MAX_CLIENTS; j++)
                {
                    if (client_socket[j] != 0) {
                        if (i == j) {
                            sprintf(msg, "Ваш заказ будет готов через %d секунд\0", sec);
                            send(client_socket[j], msg, strlen(msg), 0);
                            sec = 0;
                        }
                    }
                }
            }
        }
    }
    DeleteCriticalSection(&cs);
    WSACleanup();
}
