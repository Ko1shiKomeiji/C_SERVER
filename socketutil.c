#include "socketutil.h"

int createTCPIpv4Socket() { return socket(AF_INET, SOCK_STREAM, 0); }

struct sockaddr_in* createIPv4Address(struct hostent *ip, int port) {
    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
    address->sin_family = AF_INET;
    address->sin_port = htons(port);

    if(strlen((char *)ip) == 0)
        address->sin_addr.s_addr = INADDR_ANY;
    else
        inet_pton(AF_INET, (char *)ip, &address->sin_addr.s_addr);

    return address;
}

char* formatString(const char* name, const int session_t) {
    char* result = (char*)malloc(10 + strlen(name));
    if (result == NULL) {
        printf("ALLOCATION ERROR\n");
        return NULL;
    }
    if (session_t == 1) {
        sprintf(result, "> %s - %s\n", name, "IDLE");
    } else if (session_t == 2) {
        sprintf(result, "> %s - %s\n", name, "PLAYING");
    } else {
        sprintf(result, "> %s - %s\n", name, "CHATTING");
    }
    return result;
}

char* formatRequest(const char* name) {
    char* result = (char*)malloc(102 + strlen(name));
    if (result == NULL) {
        printf("ALLOCATION ERROR\n");
        return NULL;
    }
    sprintf(result, "YOU HAVE RECIEVED AN INVITE FROM: %s\nTYPE \"YES\" IF YOU WANT TO JOIN\n", name);
    return result;
}

int checkConnection(int socketFD) {
    char buf[128];
    int t;
    t = recv(socketFD, buf, sizeof(buf), MSG_PEEK | MSG_DONTWAIT);
    if (t == 0) return 0;
    return 1;
}

char** init_board(int board_size) {
    char** board = (char **)malloc(board_size * sizeof(char *));
    if (board == NULL) {
        printf("ALLOCATE ERROR\n");
        return NULL;
    }
    for (int i = 0; i < board_size; i++) {
        board[i] = (char *)malloc(board_size * sizeof(char));
        if (board[i] == NULL) {
            printf("ALLOCATE ERROR\n");
            return NULL;
        }
        for (int j = 0; j < board_size; j++) {
            board[i][j] = ' ';
        }
    }
    return board;
}

bool isNumber(const char *input) {
    char *endptr;
    strtol(input, &endptr, 10);
    return (*endptr == '\0' || *endptr == '\n');
}

void display_board(int board_size, char** board, int fd) {
    char buffer[100];

    send(fd, "   ", 3, 0);
    for (int i = 0; i < board_size; i++) {
        sprintf(buffer, "%2d  ", i + 1);
        send(fd, buffer, strlen(buffer), 0);
    }
    send(fd, "\n", 1, 0);
    for (int i = 0; i < board_size; i++) {
        sprintf(buffer, "%2d ", i + 1);
        send(fd, buffer, strlen(buffer), 0);
        for (int j = 0; j < board_size; j++) {
            sprintf(buffer, " %c", board[i][j]);
            send(fd, buffer, strlen(buffer), 0);
            if (j != board_size - 1) {
                send(fd, " |", 2, 0);
            }
        }
        send(fd, "\n", 1, 0);
        if (i != board_size - 1) {
            send(fd, "   ", 3, 0);
            for (int j = 0; j < board_size; j++) {
                send(fd, "---", 3, 0);
                if (j != board_size - 1) {
                    send(fd, "+", 1, 0);
                }
            }
            send(fd, "\n", 1, 0);
        }
    }
}

void freeBoard(int size, char** board) {
    for (int i = 0; i < size; i++) {
        free(board[i]);
    }
    free(board);
}

int extractNumbers(char *input, int *first_num, int *second_num) {
    char remainder[100];

    if (sscanf(input, "%d %d%s", first_num, second_num, remainder) == 2 && strlen(remainder) == 0) {
        return 1;
    } else {
        return 0;
    }
}

int check_win(char symbol, int row, int col, int board_size, char** board) {
    int i, j;
    int count;

    count = 0;
    for (j = 0; j < board_size; j++) { // Проверка по горизонтали
        if (board[row][j] == symbol) {
            count++;
        }
    }
    if (count == board_size) {
        return 1;
    }

    count = 0;
    for (i = 0; i < board_size; i++) { // Проверка по вертикали
        if (board[i][col] == symbol) {
            count++;
        }
    }
    if (count == board_size) {
        return 1;
    }

    if (row == col) { // Проверка главной диагонали
        count = 0;
        for (i = 0; i < board_size; i++) {
            if (board[i][i] == symbol) {
                count++;
            }
        }
        if (count == board_size) {
            return 1;
        }
    }

    if (row + col == board_size - 1) { // Проверка побочной диагонали
        count = 0;
        for (i = 0; i < board_size; i++) {
            if (board[i][board_size - 1 - i] == symbol) {
                count++;
            }
        }
        if (count == board_size) {
            return 1;
        }
    }
    return 0;
}

int drawCheck(char **board, int size) {
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            if (board[i][j] == ' ') return 0;
        }
    }
    return 1;
}

struct AcceptedSocket * finder(struct AcceptedSocket* clientSocket, char* name, struct AcceptedSocket **acceptedSockets, int count) {
    if (strcmp(clientSocket->name, name) == 0) {
        send(clientSocket->acceptedSocketFD, "CHOOSE ANOTHER PERSON, NOT YOURSELF\n", strlen("CHOOSE ANOTHER PERSON, NOT YOURSELF\n"), 0);
        return NULL;
    } else {
        struct AcceptedSocket* socket_rec;
        int iter = 0;
        int flag = 0;
        if (count > 10) {
            iter = 10;
        } else {
            iter = count;
        }
        for (int i = 0; i < iter; ++i) {
            if (strcmp(acceptedSockets[i]->name, name) == 0 && acceptedSockets[i]->session_t == 1 && acceptedSockets[i]->free == false) {
                flag = 1;
                socket_rec = acceptedSockets[i];
                break;
            }  
        }
        if (flag == 0) {
            send(clientSocket->acceptedSocketFD, "PERSON NOT FOUND OR PLAYING ALREADY\n", strlen("PERSON NOT FOUND OR PLAYING ALREADY\n"), 0);   
            return NULL;
        } else {
            return socket_rec;
        }
    }
}

void gameSession(struct AcceptedSocket* clientSocket, struct AcceptedSocket* enemySocket) {
    char buffer[1024];
    char symbol;
    int row = 0, col = 0;
    ssize_t amountReceived_r, amountReceived;
    if (strcmp(clientSocket->host, "") == 0) { // Если пользователь является хостом
        if (checkConnection(clientSocket->acceptedSocketFD) == 0) return; // Проверка подключения
        send(clientSocket->acceptedSocketFD, "HELLO, HOST!\nCHOOSE SIZE OF A BOARD (3 - 13)\n", strlen("HELLO, HOST!\nCHOOSE SIZE OF A BOARD (3 - 13)\n"), 0);
        int size;
        symbol = 'X';
        while (true) {
            memset(buffer, 0, 1024);
            amountReceived_r = recv(clientSocket->acceptedSocketFD, buffer, 1024, 0);
            buffer[amountReceived_r] = 0;
            if (amountReceived_r > 0) { // Если данные получены
                if (isNumber(buffer)) { // Проверка ввод на то, является ли он числом
                    size = atoi(buffer);
                    if (size >= 3 && size <= 13) break; // Проверка ввода на границы размеров поля
                }
            } else { // Если данные не получены
                clientSocket->free = true;
                clientSocket->session_t = 1;
                clientSocket->turn = -1;
                return;
            }
        }
        clientSocket->board = init_board(size); // Инициализация поля
        enemySocket->board = clientSocket->board;
        clientSocket->board_size = size;
        enemySocket->board_size = size;
    } else { // Если пользователь не хост
        if (checkConnection(clientSocket->acceptedSocketFD) == 0) return; // Проверка подключения
        send(clientSocket->acceptedSocketFD, "HELLO, JOINER!\nWAITING FOR HOST...\n", strlen("HELLO, JOINER!\nWAITING FOR HOST...\n"), 0);
        symbol = 'O';
        while (!enemySocket->board_size) { // Ожидание выбора размера поля хостом
            if (enemySocket->free == true) { // Если противник вышел
                if (checkConnection(clientSocket->acceptedSocketFD) != 0) send(clientSocket->acceptedSocketFD, "OPPONENT HAS LEFT. YOU WIN!\n", strlen("OPPONENT HAS LEFT. YOU WIN!\n"), 0);
                clientSocket->session_t = 1;
                clientSocket->turn = -1;
                clientSocket->board_size = 0;
                return;
            }
            if (checkConnection(clientSocket->acceptedSocketFD) == 0) { // Проверка подключения
                clientSocket->session_t = 1;
                clientSocket->free = true;
                clientSocket->turn = -1;
                clientSocket->board_size = 0;
                return;
            }
        };
    }
    while (true) {
        if (enemySocket->free == true) { // Если пользователь вышел из игры
            if (checkConnection(clientSocket->acceptedSocketFD) == 0) return;
            send(clientSocket->acceptedSocketFD, "OPPONENT HAS LEFT. YOU WIN!\n", strlen("OPPONENT HAS LEFT. YOU WIN!\n"), 0);
            display_board(clientSocket->board_size, clientSocket->board, clientSocket->acceptedSocketFD);
            clientSocket->session_t = 1;
            clientSocket->turn = -1;
            clientSocket->board_size = 0;
            return;
        }
        if (enemySocket->session_t == 1) { // Если противник победил и вышел из лобби
            if (checkConnection(clientSocket->acceptedSocketFD) == 0) return;
            display_board(clientSocket->board_size, clientSocket->board, clientSocket->acceptedSocketFD);
            clientSocket->session_t = 1;
            clientSocket->turn = -1;
            clientSocket->board_size = 0;
            return;
        }
        if (clientSocket->turn == 1) { // Если ход
            if (drawCheck(clientSocket->board, clientSocket->board_size)) { // Проверка ничьи
                if (checkConnection(clientSocket->acceptedSocketFD) == 0) return;
                send(enemySocket->acceptedSocketFD, "DRAW!\n", strlen("DRAW!\n"), 0);
                if (checkConnection(clientSocket->acceptedSocketFD) == 0) return;
                send(clientSocket->acceptedSocketFD, "DRAW!\n", strlen("DRAW!\n"), 0);
                clientSocket->session_t = 1;
                clientSocket->turn = -1;
                clientSocket->board_size = 0;
                enemySocket->board_size = 0;
                enemySocket->turn = -1;
                enemySocket->session_t = 1;
                return;
            }
            
            amountReceived = recv(clientSocket->acceptedSocketFD, buffer, 1024, MSG_DONTWAIT); // Подчищаем данные в дескрипторе
            if (amountReceived > 0) {
                memset(buffer, 0, 1024);
            } else if (amountReceived == 0) {
                clientSocket->session_t = 1;
                clientSocket->turn = -1;
                clientSocket->board_size = 0;
                clientSocket->free = true;
                return;
            }
            send(clientSocket->acceptedSocketFD, "YOUR TURN!\n", strlen("YOUR TURN!\n"), 0);

            if (checkConnection(clientSocket->acceptedSocketFD) == 0) return;
            display_board(clientSocket->board_size, clientSocket->board, clientSocket->acceptedSocketFD);

            while (true) {
                memset(buffer, 0, 1024);
                amountReceived = recv(clientSocket->acceptedSocketFD, buffer, 1024, 0);
                if (amountReceived > 0) { // Если получили данные
                    if (extractNumbers(buffer, &row, &col) == 1) { // Проверяем корректность ввода
                        if (row > 0 && row <= clientSocket->board_size && col > 0 && col <= clientSocket->board_size && clientSocket->board[row - 1][col - 1] == ' ') { 
                            clientSocket->board[row - 1][col - 1] = symbol;
                            if (check_win(symbol, row - 1, col - 1, clientSocket->board_size, clientSocket->board)) { // Проверяем победу
                                if (checkConnection(clientSocket->acceptedSocketFD) == 0) return;
                                display_board(clientSocket->board_size, clientSocket->board, clientSocket->acceptedSocketFD);
                                send(clientSocket->acceptedSocketFD, "YOU WON! CONGRATULATIONS!\n", strlen("YOU WON! CONGRATULATIONS!\n"), 0);
                                if (checkConnection(enemySocket->acceptedSocketFD) != 0) 
                                    send(enemySocket->acceptedSocketFD, "YOU LOSE!\n", strlen("YOU LOSE!\n"), 0);
                                clientSocket->session_t = 1;
                                clientSocket->turn = -1;
                                clientSocket->board_size = 0;
                                return;
                            }
                            break;
                        } else {
                            continue;
                        }
                    }
                } else if (amountReceived == 0) { // Если не получили данные
                    clientSocket->free = true;
                    clientSocket->session_t = 1;
                    clientSocket->turn = -1;
                    clientSocket->board_size = 0;
                    return;
                }
            }
            if (checkConnection(enemySocket->acceptedSocketFD) == 0) { // Если пользователь вышел из игры
                if (checkConnection(clientSocket->acceptedSocketFD) == 0) return;
                send(clientSocket->acceptedSocketFD, "OPPONENT HAS LEFT. YOU WIN!\n", strlen("OPPONENT HAS LEFT. YOU WIN!\n"), 0);
                display_board(clientSocket->board_size, clientSocket->board, clientSocket->acceptedSocketFD);
                clientSocket->session_t = 1;
                clientSocket->turn = -1;
                clientSocket->board_size = 0;
                return;
            }
            display_board(clientSocket->board_size, clientSocket->board, clientSocket->acceptedSocketFD);
            clientSocket->turn = 0;
            enemySocket->turn = 1;
        }
    }
}