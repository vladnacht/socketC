#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <ctype.h>

#define MAX_BUFFER_SIZE 1024

void error(char *msg) {
    perror(msg);
    exit(1);
}

void* handle_client(void *client_socket_ptr) {
    int clientSocket = *(int*)client_socket_ptr;
    free(client_socket_ptr);

    char buffer[MAX_BUFFER_SIZE];
    int bytesRead;

    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        printf("Données reçues: %.*s", bytesRead, buffer);
        
        for (int i = 0; i < strlen(buffer); i++) {
            buffer[i] = toupper(buffer[i]);
        }

        send(clientSocket, buffer, bytesRead, 0);
        printf("Données renvoyées au client.\n");
    }

    if (bytesRead == 0) {
        printf("Connexion fermée par le client.\n");
    } else {
        error("Erreur lors de la reception des données.\n");
    }

    close(clientSocket);
    return NULL;
}

int main(int argc, char *argv[]) {
    int serverSocket, *clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    pthread_t thread_id;

    if (argc < 2) {
        fprintf(stderr, "Error, no port provided\n");
        exit(1);
    }

    int port = atoi(argv[1]);

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number: %s \n", argv[1]);
        exit(EXIT_FAILURE);
    }

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        error("Erreur lors de la création de la socket");
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        error("Erreur lors de l'attachement de la socket à l'adresse et au port ");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 5) == -1) {
        error("Erreur lors de l'attente de connexions");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("Serveur echo démarré sur le port %d...\n", port);

    while (1) {
        clientSocket = malloc(sizeof(int));
        if ((*clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen)) == -1) {
            error("Erreur lors de l'acceptation de la connexion client");
            close(serverSocket);
            exit(EXIT_FAILURE);
        }

        printf("Nouvelle connexion client depuis %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        if (pthread_create(&thread_id, NULL, handle_client, clientSocket) != 0) {
            error("Erreur lors de la création du thread");
            close(*clientSocket);
            free(clientSocket);
        }
        pthread_detach(thread_id);
    }

    close(serverSocket);
    return 0;
}
