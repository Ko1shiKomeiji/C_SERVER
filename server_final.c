#include <pthread.h>
#include "socketutil.h"

struct AcceptedSocket * acceptIncomingConnection(int serverSocketFD);

void acceptNewConnectionAndReceiveAndPrintItsData(int serverSocketFD);

void session(void* clientSocket_s);

void startAcceptingIncomingConnections(int serverSocketFD);

struct AcceptedSocket *acceptedSockets[10];
int acceptedSocketsCount = 0;


void startAcceptingIncomingConnections(int serverSocketFD) {
    while(true) {
        struct AcceptedSocket *clientSocket = acceptIncomingConnection(serverSocketFD);
        if (acceptedSocketsCount >= 10) { // Если суммарное количество подключений к серверу >10
            for (int i = 0; i < 10; ++i) {
                if (acceptedSockets[i]->free == true) { // Ищем первую свободную структуру и заменяем её
                    free(acceptedSockets[i]);
                    acceptedSockets[i] = clientSocket;
                    break;
                }
            }
        } else {
            acceptedSockets[acceptedSocketsCount++] = clientSocket;
        }

        pthread_t id;
        pthread_create(&id, NULL, (void *)session, (void *)clientSocket); // Запускаем сессию в отдельном потоке
    }
}

void session(void* clientSocket_s) {
    struct AcceptedSocket *clientSocket = (struct AcceptedSocket *)clientSocket_s; // Приведение корректного типа (Чтобы компилятор не возникал)
    char buffer[1024];
    send(clientSocket->acceptedSocketFD, HELLO_MSG, strlen(HELLO_MSG), 0);
    while (true) {
        memset(buffer, 0, 1024);
        ssize_t amountReceived = recv(clientSocket->acceptedSocketFD, buffer, 1024, 0);

        if (amountReceived > 0) { // Если данные получены, запускаем часть, отвечающую за ввод никнейма
            buffer[amountReceived] = 0;
            int flag = 0;
            int iter = 0;
            if (acceptedSocketsCount > 10) { // Проверяем, если суммарное число подключений на сервер > 10 (Все структуры инициализированы), то пробегаемся по ним
                iter = 10;
            } else {
                iter = acceptedSocketsCount;
            }
            if (iter > 1) { // Если человек не первый в системе
                for (int i = 0; i < iter - 1; ++i) { // Пробегаемся по всем структурам ДО текущей (Поэтому -1)
                    if (strcmp(acceptedSockets[i]->name, buffer) == 0 && acceptedSockets[i]->free == false) { // Проверяем среди активных пользователей
                        send(clientSocket->acceptedSocketFD, "NAME WAS ALREADY TAKEN, PLEASE TRY ANOTHER ONE\n", strlen("NAME WAS ALREADY TAKEN, PLEASE TRY ANOTHER ONE\n"), 0);
                        flag = 1;
                        break;
                    }
                }
            }
            if (flag == 0) { // Присваиваем имя
                strcpy(clientSocket->name, buffer);
                char *name;
                name = (char *)malloc(strlen(buffer) + 8);
                sprintf(name, "HELLO, %s\n", buffer);
                send(clientSocket->acceptedSocketFD, name, strlen(name), 0);
                printf("JOINED: %s\n", clientSocket->name);
                free(name);
                break;
            }
        }

        if (amountReceived == 0) { // Если соединение было нарушено, то выходим из сессии
            break;            
        }

    }

    while (true) {  // Основная часть сессии
        memset(buffer, 0, 1024);
        send(clientSocket->acceptedSocketFD, INTERFACE_DATA, strlen(INTERFACE_DATA), 0); // Отправляем интерфейс
        ssize_t amountReceived = recv(clientSocket->acceptedSocketFD, buffer, 1024, 0);

        if (amountReceived > 0) { // Если получили данные
            buffer[amountReceived] = 0;
            if (clientSocket->session_t == 3) { // Если было получено приглашение, то обрабатываем ответ (тип сессии "отвечающий")
                struct AcceptedSocket *socket_rec;
                if (strcmp("YES", buffer) == 0) { // Получен положительный ответ на приглашение
                    strcpy(clientSocket->session_ans, "YES"); // Добавляем наш ответ в поле структуры, чтобы хост мог обработать наш ответ
                    clientSocket->session_t = 2; // Меняем тип сессии на "игровой"
                    clientSocket->turn = 0; // Кладем в поле структуры информацию о том, что ходим не первыми
                    int iter = 0; 
                    if (acceptedSocketsCount > 10) { // Проверяем, если суммарное число подключений на сервер > 10 (Все структуры инициализированы), то пробегаемся по ним
                        iter = 10;
                    } else {
                        iter = acceptedSocketsCount;
                    }
                    for(int i = 0 ; i < iter; ++i) { // Находим структуру хоста
                        if (strcmp(acceptedSockets[i]->name, clientSocket->host) == 0) {
                            socket_rec = acceptedSockets[i];
                        }
                    }
                    gameSession(clientSocket, socket_rec); // Запускаем сессию со стороны клиента
                    memset(clientSocket->host, 0, sizeof(clientSocket->host));
                    if (clientSocket->free == true) break; // Если пользователь вышел во время игры, то помечаем его структуру, как свободную
                } else { // Если ответ на приглашение не положительный
                    sprintf(clientSocket->session_ans, "NO"); // Добавляем наш ответ в поле структуры, чтобы хост мог обработать наш ответ 
                    clientSocket->session_t = 1; // Меняем тип сессии на "обычный"
                    memset(clientSocket->host, 0, sizeof(clientSocket->host)); // Удаляем имя хоста из поля 
                }
            }

            if (strcmp(buffer, "1") == 0) { 
                memset(buffer, 0, 1024);
                send(clientSocket->acceptedSocketFD, "WHO DO YOU WANT TO INVITE?\n", strlen("WHO DO YOU WANT TO INVITE?\n"), 0);
                ssize_t amountReceived_r = recv(clientSocket->acceptedSocketFD, buffer, 1024, 0);
                if (amountReceived_r > 0) { // Если получили данные
                    struct AcceptedSocket *socket_rec = finder(clientSocket, buffer, acceptedSockets, acceptedSocketsCount); // Запускаем функцию поиска получателя сообщения
                    if (socket_rec != NULL) { // Если нашли такого пользователя
                        socket_rec->session_t = 3; // Меняем тип сессии получателя на "отвечающий"
                        sprintf(socket_rec->host, clientSocket->name); // Добавляем имя в поле "хост" получателя
                        memset(buffer, 0, 1024);
                        char* request = formatRequest(clientSocket->name); // Запускаем функцию формирования текста приглашения

                        send(clientSocket->acceptedSocketFD, "REQUEST HAS SENT!\n", strlen("REQUEST HAS SENT!\n"), 0); 
                        send(socket_rec->acceptedSocketFD, request, strlen(request), 0); // Отправляем приглашение получателю
                        free(request);

                        while (strcmp(socket_rec->session_ans, "") == 0 && socket_rec->free == false); // Ждём ответа или выхода получателя

                        if (checkConnection(clientSocket->acceptedSocketFD) == 0) { // Если пользователь вышел за время ожидания ответа от получателя
                            socket_rec->session_t = 1; 
                            socket_rec->turn = -1;
                            memset(socket_rec->session_ans, 0, sizeof(socket_rec->session_ans));
                            memset(socket_rec->host, 0, sizeof(socket_rec->host));
                            break; // Выходим из сессии
                        }

                        if (strcmp("YES", socket_rec->session_ans) == 0) { // Если получили положительный ответ
                            send(clientSocket->acceptedSocketFD, "JOINING GAME!\n", strlen("JOINING GAME!\n"), 0);
                            clientSocket->turn = 1; // Кладем в поле структуры информацию о том, что ходим первыми
                            memset(socket_rec->session_ans, 0, sizeof(socket_rec->session_ans));
                            clientSocket->session_t = 2; // Меняем тип сессии на "игровой"
                            gameSession(clientSocket, socket_rec); // Запускаем сессию со стороны хоста
                            if (checkConnection(clientSocket->acceptedSocketFD) == 0) { // Если пользователь вышел во время игры
                                break;
                            }
                            while (socket_rec->session_t != 1); // Ждём выхода оппонента
                            freeBoard(clientSocket->board_size, clientSocket->board); // Освобождаем память, занятую полем
                            clientSocket->board = NULL; 
                            socket_rec->board = NULL;
                        } else if (strcmp("NO", socket_rec->session_ans) == 0) { // Если не получили положительный ответ
                            send(clientSocket->acceptedSocketFD, "INVITE WAS REJECTED BY USER!\n", strlen("INVITE WAS REJECTED BY USER!\n"), 0);
                            memset(socket_rec->session_ans, 0, sizeof(socket_rec->session_ans));
                            continue;

                        } else if (socket_rec->free == false) { // Если получатель вышел
                            if (checkConnection(clientSocket->acceptedSocketFD) == 0) break;
                            send(clientSocket->acceptedSocketFD, "ERROR\n", strlen("ERROR\n"), 0);

                        }
                        socket_rec = NULL;
                    }

                } else if (amountReceived_r == 0) { // Если не получили данные
                    break;
                }
                
            }

            if (strcmp(buffer, "2") == 0) { 
                int iter = 0;
                if (acceptedSocketsCount > 10) { // Проверяем, если суммарное число подключений на сервер > 10 (Все структуры инициализированы), то пробегаемся по ним
                    iter = 10;
                } else {
                    iter = acceptedSocketsCount;
                }
                for(int i = 0 ; i < iter; i++) {
                    if (acceptedSockets[i]->free == false) {
                        printf("NAME: %s - SESSION TYPE: %d - HOST: %s\n", acceptedSockets[i]->name, acceptedSockets[i]->session_t, acceptedSockets[i]->host);
                        char* name = formatString(acceptedSockets[i]->name, acceptedSockets[i]->session_t);
                        send(clientSocket->acceptedSocketFD, name, strlen(name), 0); // Выводим информацию о пользователях в сети
                        free(name);
                    }
                }
            }

            if (strcmp(buffer, "3") == 0) {
                clientSocket->session_t = 4; // Меняем тип сессии на "чат"
                struct AcceptedSocket *socket_rec;
                send(clientSocket->acceptedSocketFD, "TO WHOM DO YOU WANT TO SEND A MESSAGE?\n", strlen("TO WHOM DO YOU WANT TO SEND A MESSAGE?\n"), 0);
                memset(buffer, 0, 1024);
                ssize_t amountReceived_r = recv(clientSocket->acceptedSocketFD, buffer, 1024, 0);
                if (amountReceived_r > 0) // Если получили данные
                    socket_rec = finder(clientSocket, buffer, acceptedSockets, acceptedSocketsCount);
                else if (amountReceived_r == 0)
                    break;
                if (socket_rec != NULL) { // Если нашли получателя
                    memset(buffer, 0, 1024);
                    send(clientSocket->acceptedSocketFD, "WRITE A MESSAGE:\n", strlen("WRITE A MESSAGE:\n"), 0);
                    amountReceived_r = recv(clientSocket->acceptedSocketFD, buffer, 1024, 0);
                    if (amountReceived_r > 0) { // Если получили данные
                        char* send_msg = malloc(strlen(clientSocket->name) + strlen(buffer) + 4);
                        sprintf(send_msg, "%s: %s\n", clientSocket->name, buffer);
                        if (socket_rec->free == false) { // Если пользователь всё ещё в сети
                            send(socket_rec->acceptedSocketFD, send_msg, strlen(send_msg), 0);
                        }
                        free(send_msg);
                    } else if (amountReceived_r == 0) {
                        break;
                    }
                }
                clientSocket->session_t = 1; // Меняем тип сессии на "обычный"
            }
        } else if (amountReceived == 0)
            break;
    }

    if (clientSocket->free == false) 
        clientSocket->free = true;
    if (strlen(clientSocket->name) != 0)
        printf("LEFT: %s\n", clientSocket->name);
    memset(clientSocket->name, 0, sizeof(clientSocket->name));
    close(clientSocket->acceptedSocketFD);
}

struct AcceptedSocket * acceptIncomingConnection(int serverSocketFD) {
    struct sockaddr_in  clientAddress;
    int clientAddressSize = sizeof (struct sockaddr_in);
    int clientSocketFD = accept(serverSocketFD, (struct sockaddr *)&clientAddress, (socklen_t *)&clientAddressSize);

    struct AcceptedSocket* acceptedSocket = malloc(sizeof (struct AcceptedSocket));
    acceptedSocket->address = clientAddress;
    acceptedSocket->acceptedSocketFD = clientSocketFD;
    acceptedSocket->free = false;
    acceptedSocket->session_t = 1;
    acceptedSocket->turn = -1;
    acceptedSocket->board = NULL;
    memset(acceptedSocket->host, 0, sizeof(acceptedSocket->host));

    return acceptedSocket;
}


int main() {
    int serverSocketFD = createTCPIpv4Socket(0);
    struct sockaddr_in *serverAddress = createIPv4Address((struct hostent*)"", 2000);

    int result = bind(serverSocketFD, (struct sockaddr*)serverAddress, sizeof(*serverAddress));
    if (result == 0)
        printf("SOCKET WAS BOUND SUCCESSFULLY\n");
    else {
        printf("CANNOT BIND SOCKET\n");
        _exit(1);
    }

    listen(serverSocketFD, 10);

    startAcceptingIncomingConnections(serverSocketFD);

}