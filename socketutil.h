#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define HELLO_MSG "HELLO TO TIC-TAC-TOE GAME ver 0.1!\nPLEASE ENTER YOUR NAME SO WE CAN IDENTIFY YOU IN THE SYSTEM\n"
#define INTERFACE_DATA "YOU CAN:\n\n   1 - START GAME WITH SOMEBODY\n\n   2 - WATCH LIST OF ONLINE PLAYERS\n\n   3 - SEND MESSAGE TO SOMEONE\n"

struct AcceptedSocket {
    int acceptedSocketFD;
    struct sockaddr_in address;
    char name[1024];
    bool free;
    /*
        1 - обычный тип сессии
        2 - игровой тип сессии
        3 - отвечающий тип сессии
        4 - тип сессии чат
    */
    int session_t;
    char session_ans[4];
    /*
        -1 - not playing
        0 - joining
        1 - hosting
    */
    char host[1024];
    /*
        -1 - not playing
        1 - your turn
        2 - enemy turn
    */
    int turn;
    char **board;
    int board_size;
};

struct sockaddr_in* createIPv4Address(struct hostent *ip, int port); // Создание адреса

int createTCPIpv4Socket(); // Создание сокета

char* formatString(const char* name, const int session_t); // Форматирование вывода списка пользователей

char* formatRequest(const char* name); // Форматирование пришлашения в игру

int checkConnection(int socketFD); // Проверка подключения

char** init_board(int board_size); // Инициализация поля

bool isNumber(const char *input); // Проверка ввода на число

void display_board(int board_size, char** board, int fd); // Отображение поля

void freeBoard(int size, char** board); // Освобождение памяти, занятой полем

int extractNumbers(char *input, int *first_num, int *second_num); // Проверка ввода в игре

int check_win(char symbol, int row, int col, int board_size, char** board); // Проверка на победу

int drawCheck(char **board, int size); // Проверка на ничью

struct AcceptedSocket * finder(struct AcceptedSocket* clientSocket, char* name, struct AcceptedSocket **acceptedSockets, int count); // Поиск получателя

void gameSession(struct AcceptedSocket* clientSocket, struct AcceptedSocket* enemySocket); // Игровая сессия