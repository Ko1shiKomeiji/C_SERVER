#include <pthread.h>
#include "socketutil.h"


void startListeningAndPrintMessagesOnNewThread(void *socketFD);

void listenAndPrint(void *socketFD);

void readConsoleEntriesAndSendToServer(int socketFD);

int main(int argc, char* argv[]) {
    int sock;
    struct sockaddr_in server;
    struct hostent *hp;

    if (argc < 3) {
        printf("USAGE: ./client HOSTNAME PORT\n");
        _exit(1);
    }
    
    if((sock = socket( AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("opening stream socket");
        exit(1);
    }

    hp=malloc(sizeof(struct hostent));

    server.sin_family = AF_INET;
    hp = gethostbyname(argv[1]);
    if (hp == (struct hostent *) 0) {
        fprintf(stderr, "%s: unknown host\n", argv[1]);
        exit(2);
    }

    memcpy((char *) &server.sin_addr, (char *) hp->h_addr, hp->h_length);
    server.sin_port = htons(atoi(argv[2]));
    if (connect(sock, (struct sockaddr *) &server, sizeof server) == -1) {
        printf("COULDN'T CONNECT TO THE SERVER!\n");
        exit(1);
    } else {
        printf("CONNECTION SUCCESSFUL\n\n");
    }

    startListeningAndPrintMessagesOnNewThread((void *)&sock);
    readConsoleEntriesAndSendToServer(sock);


    close(sock);
    return 0;
}

void readConsoleEntriesAndSendToServer(int socketFD) {
    char *line = NULL;
    size_t lineSize= 0;

    while(true) {
        ssize_t  charCount = getline(&line, &lineSize, stdin);
        line[charCount - 1] = 0;

        if(charCount > 0) {
            if (strcmp(line, "exit") == 0) break;
            send(socketFD, line, strlen(line), 0);
        }
    }
}

void startListeningAndPrintMessagesOnNewThread(void *socketFD) {
    pthread_t id ;
    pthread_create(&id, NULL, (void *)listenAndPrint, socketFD);
}

void listenAndPrint(void *socket) {
    int *socketFD = (int *)socket;
    char buffer[4096];

    while (true) {   
        memset(buffer, 0, 4096);
        ssize_t  amountReceived = recv(*socketFD, buffer, 4096, 0);

        if(amountReceived > 0) {
            buffer[amountReceived] = 0;
            printf("%s",buffer);
        }

        if(amountReceived==0)
            break;
    }

    close(*socketFD);
}