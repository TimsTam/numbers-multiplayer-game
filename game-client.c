// The Game Client handles user input and displayes messages from the server. 
// The client also connects to the Game Server, receives game messages, and sends player moves.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFFER 256

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <Game Type> <Server IP> <Port Number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    WSADATA wsaData;
    SOCKET client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER];
    int bytes_received;

    // initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    // create socket for client
    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // set server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[2]);
    server_addr.sin_port = htons(atoi(argv[3]));

    // connect to server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Connect failed: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // get and display the welcome message
    bytes_received = recv(client_socket, buffer, BUFFER - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0'; // Null-terminate the string
        printf("%s", buffer);
    }

    // main game loop
    while (1) {
        // get messages from server
        bytes_received = recv(client_socket, buffer, BUFFER - 1, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("Server disconnected.\n");
            } else {
                fprintf(stderr, "Recv failed: %d\n", WSAGetLastError());
            }
            break;
        }

        buffer[bytes_received] = '\0';
        printf("%s", buffer);

        // check if game is over or if player has won
        if (strstr(buffer, "You won!") != NULL || strstr(buffer, "Game Over!") != NULL) {
            break;  // exit loop if game is over
        }

        // check if its the players turn
        if (strstr(buffer, "It's your turn!") != NULL || strstr(buffer, "Try again!") != NULL) {
            while (1) {
                if (fgets(buffer, BUFFER, stdin) != NULL) { // get user input for turn
                    buffer[strcspn(buffer, "\r\n")] = 0;

                    // send user input to server
                    send(client_socket, buffer, strlen(buffer), 0);
                    break; // exit input loop
                }
            }
        } 
    }
    // clear up resources
    closesocket(client_socket);
    WSACleanup();
    return 0;
}
