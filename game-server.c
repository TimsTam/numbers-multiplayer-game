// The Game Server listens for incoming player connections, manages game state and also coordinates player turns. 
// It uses multi-threading to handle multiple players at the same time.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <pthread.h>
#include <stdarg.h>

#pragma comment(lib, "Ws2_32.lib") // linking Winsock library

#define BUFFER 256 // size of buffer used for communication between client and server
#define TIMEOUT 20 // timeout duration

// struct to store player info
typedef struct {
    SOCKET client_socket; // Socket associated with the player
    int player_id; // Unique ID assigned to each player
} player_t;

// struct to store game info
typedef struct {
    int port; // port server will listen on
    int player_count;
    int current_total;
    int joined_count;
    player_t *players; // array of player structures
    int *active_players; // array to track active players (1 for active, 0 for inactive)
    int current_turn; // index of the player whose turn it is
} game_t;

game_t game; // game struct

// function declarations
void *handle_client(void *arg);
void send_to_all_clients(const char *format, const char *tag, ...);
void send_turn_notification();
void check_game_status();
void end_game();
void send_tagged_message(SOCKET client_socket, const char *tag, const char *message);

int main(int argc, char *argv[]) {
    // check for correct number of arguments
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <Port Number> <Game Type> <Number of Players>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // initializing game parameters
    game.port = atoi(argv[1]);
    game.player_count = atoi(argv[3]);
    game.current_total = 25;
    game.joined_count = 0;
    game.players = malloc(game.player_count * sizeof(player_t)); // allocate memory for player array
    game.active_players = malloc(game.player_count * sizeof(int)); // allocate memory for active players array
    game.current_turn = 0; // Set the first player's turn

    // check if memory allocation fails
    if (!game.players || !game.active_players) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // set all players to active
    for (int i = 0; i < game.player_count; ++i) {
        game.active_players[i] = 1;
    }

    WSADATA wsaData;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);

    // initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    // create a socket for the server
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(game.port);

    // bind the socket to the specified port and address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // listen for player connections joining the game
    if (listen(server_socket, game.player_count) == SOCKET_ERROR) {
        fprintf(stderr, "Listen failed: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to join...\n");

    // accept incoming player connections until all players have joined
    while (game.joined_count < game.player_count) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET) {
            fprintf(stderr, "Accept failed: %d\n", WSAGetLastError());
            continue; // continue accepting if there's an error
        }

        // set player info
        player_t *new_player = &game.players[game.joined_count];
        new_player->client_socket = client_socket;
        new_player->player_id = game.joined_count;

        game.joined_count++;

        char welcome_msg[] = "Welcome to the game\n";
        send(client_socket, welcome_msg, strlen(welcome_msg), 0);

        // once all players have joined, start the game
        if (game.joined_count == game.player_count) {
            send_to_all_clients("All players have joined. The game is starting...\n", "TEXT");
            send_turn_notification(); // notify first player of their turn

            pthread_t threads[game.player_count]; // array of thread IDs for each player

            // create a thread to handle each player
            for (int i = 0; i < game.player_count; ++i) {
                pthread_create(&threads[i], NULL, handle_client, &game.players[i]);
            }

            // wait for all threads to finish
            for (int i = 0; i < game.player_count; ++i) {
                pthread_join(threads[i], NULL);
            }
            break;
        }
    }

    // clear up resources
    closesocket(server_socket);
    WSACleanup();
    free(game.players);
    free(game.active_players);
    return 0;
}

void send_to_all_clients(const char *format, const char *tag, ...) {
    char message[BUFFER];
    va_list args;

    va_start(args, tag);
    vsnprintf(message, BUFFER, format, args);
    va_end(args);

    // output message to all active clients server to log each
    for (int i = 0; i < game.player_count; ++i) {
        if (game.active_players[i]) {
            send_tagged_message(game.players[i].client_socket, tag, message);
        }
    }
}


void send_tagged_message(SOCKET client_socket, const char *tag, const char *message) {
    char tagged_message[BUFFER];

    // find player number corresponding to the socket
    int player_id = -1;
    for (int i = 0; i < game.player_count; ++i) {
        if (game.players[i].client_socket == client_socket) {
            player_id = i;
            break;
        }
    }

    snprintf(tagged_message, BUFFER, "%s %s", tag, message);

    // send plain message to client
    send(client_socket, message, strlen(message), 0);

    // send tagged message to server
    if (player_id != -1) {
        printf("Server to Client %d: %s", player_id + 1, tagged_message);
    } else {
        printf("Server to unknown client: %s", tagged_message);
    }
}

void send_turn_notification() {
    char turn_msg[BUFFER];
    snprintf(turn_msg, BUFFER, "It's your turn!\n");
    for (int i = 0; i < game.player_count; ++i) {
        if (game.active_players[i]) {
            if (i == game.current_turn) {
                send(game.players[i].client_socket, turn_msg, strlen(turn_msg), 0);
            } else {
                send(game.players[i].client_socket, "\nServer: Waiting for other players...\n", 40, 0);
            }
        }
    }

    // send "GO" message to server
    printf("Server to Client %d: GO\n", game.current_turn + 1);
}

void check_game_status() {
    int active_count = 0;
    int last_active_player = -1;

    for (int i = 0; i < game.player_count; ++i) {
        if (game.active_players[i]) {
            active_count++;
            last_active_player = i;
        }
    }

    if (active_count == 1) {
        // send winner message
        if (last_active_player != -1) {
            send_tagged_message(game.players[last_active_player].client_socket, "TEXT", "You won!\n");

            // send loser message to players
            for (int i = 0; i < game.player_count; ++i) {
                if (i != last_active_player && game.active_players[i]) {
                    send_tagged_message(game.players[i].client_socket, "TEXT", "You lost!\n");
                }
            }
        }
        end_game(); // end game
    } else {
        // move turn to next active player
        int original_turn = game.current_turn;
        do {
            game.current_turn = (game.current_turn + 1) % game.player_count;
        } while (!game.active_players[game.current_turn] && game.current_turn != original_turn);
        
        send_turn_notification();
    }
}


void end_game() {
    char end_msg[BUFFER];

    // Send end message to all active players
    for (int i = 0; i < game.player_count; ++i) {
        if (game.active_players[i]) {
            snprintf(end_msg, BUFFER, "Server to Client %d: END\n", i + 1);
            printf(end_msg);
            
            // close client socket
            closesocket(game.players[i].client_socket);
            game.active_players[i] = 0;
        }
    }
    exit(0); // terminate server
}

void *handle_client(void *arg) {
    player_t *player = (player_t *)arg;
    char buffer[BUFFER];
    int bytes_received;
    int invalid_attempts = 0;

    while (1) {
        fd_set read_fds;
        struct timeval tv;
        FD_ZERO(&read_fds);
        FD_SET(player->client_socket, &read_fds);
        tv.tv_sec = TIMEOUT;
        tv.tv_usec = 0;

        int activity = select(0, &read_fds, NULL, NULL, &tv);

        if (activity == 0) { // when timeout occurs, remove player from game
            printf("TEXT Player %d timed out.\n", player->player_id + 1);
            send_tagged_message(player->client_socket, "TEXT", "You Lost!\n");
            char end_msg[BUFFER];
            snprintf(end_msg, BUFFER, "Server to Client %d: END\n", player->player_id + 1);
            printf(end_msg);
            closesocket(player->client_socket);
            game.active_players[player->player_id] = 0;
            check_game_status();
            return NULL;
        }

        if (activity == SOCKET_ERROR) {
            fprintf(stderr, "Select failed: %d\n", WSAGetLastError());
            return NULL;
        }

        bytes_received = recv(player->client_socket, buffer, BUFFER - 1, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                printf("TEXT Player %d disconnected.\n", player->player_id);
            } else {
                fprintf(stderr, "Recv failed: %d\n", WSAGetLastError());
            }
            game.active_players[player->player_id] = 0;
            check_game_status();
            return NULL;
        }

        buffer[bytes_received] = '\0'; // Null-terminate the received data

        // print out move to server
        printf("Client %d to Server: MOVE %s\n", player->player_id + 1, buffer);

        buffer[strcspn(buffer, "\r\n")] = 0; // remove newline and return characters

        // check if quit is entered by player. if so, remove them from the game.
        if (_stricmp(buffer, "QUIT") == 0) {
            char end_msg[BUFFER];
            snprintf(end_msg, BUFFER, "Server to Client %d: END\n", player->player_id + 1);
            printf(end_msg);
            closesocket(player->client_socket);
            game.active_players[player->player_id] = 0;
            check_game_status();
            return NULL;
        }

        int move = atoi(buffer); // ensure input is a valid number
        if (move > 0 && move < 10) {
            invalid_attempts = 0; // reset valid attempts on new move
            game.current_total -= move;

            if (game.current_total <= 0) {
                // send winner message
                send_tagged_message(game.players[game.current_turn].client_socket, "TEXT", "You won!\n");

                // send loser messages
                for (int i = 0; i < game.player_count; ++i) {
                    if (i != game.current_turn && game.active_players[i]) {
                        send_tagged_message(game.players[i].client_socket, "TEXT", "You lost!\n");
                    }
                }

                end_game(); // end game
            } else {
                // send new total to all players and server
                send_to_all_clients("Total is %d. Enter Number:\n", "TEXT", game.current_total);

                // next turn
                game.current_turn = (game.current_turn + 1) % game.player_count;
                while (!game.active_players[game.current_turn]) {
                    game.current_turn = (game.current_turn + 1) % game.player_count;
                }
                send_turn_notification();
            }
        } else {
            invalid_attempts++;
            send_tagged_message(player->client_socket, "TEXT", "ERROR Invalid move or command. Try again!\n");

            if (invalid_attempts >= 5) { // when player has made 5 invalid attempts, they are kicked out of the game
                send_tagged_message(player->client_socket, "TEXT", "You have made 5 invalid input attempts. You Lost!\n");
                char end_msg[BUFFER];
                snprintf(end_msg, BUFFER, "Server to Client %d: END\n", player->player_id + 1);
                printf(end_msg);
                closesocket(player->client_socket);
                game.active_players[player->player_id] = 0;
                check_game_status();
                return NULL;
            }
        }
    }

    return NULL;
}




