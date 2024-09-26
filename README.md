# Game Server and Client

## Overview

There are two parts to this project:

1. **Game Server**: A server program that handles player communication, connection and game logic.
2. **Game Client**: A client program that connects to the game server, receives game updates, and sends player moves.

## Game Server

### Description

The Game Server listens for incoming player connections, manages game state and also coordinates player turns. It uses multi-threading to handle multiple players at the same time.

### Features

- Accepts all player connections to number of players specified.
- Manages game state & player turns.
- Notifies clients when its their turn and gives game status updates to all players.
- Handles player disconnections and timeouts.
- Ends the game, outputting winners and losers messages.

### Compilation

To compile the server, use this command:

gcc -o game-server game-server.c -lws2_32 -lpthread

### Usage
Run the server with this command:

./game-server <Port Number> <Game Name> <Number of Players>

- <Port Number>: Port which the server will listen on.
- <Game Name>: Name or type of game being setup.
- <Number of Players>: Number of players wanted for the game.

Example: ./game-server 8080 numbers 2

## Game Client
### Description
The Game Client handles user input and displayes messages from the server. The client also connects to the Game Server, receives game messages, and sends player moves.

### Features
- Joins the game server, through ip address and port number.
- Receives and shows all game messages.
- Sends player moves from client to the server.

### Compilation
To compile the client, use this command:

gcc -o game-client game-client.c -lws2_32

### Usage
To run the client, use this command:

./game-client <Game Name> <Server IP> <Port Number>

- <Game Name>: Name or type of game.
- <Server IP>: IP address of server.
- <Port Number>: Port which the server is listening on.

Example: ./game-client numbers 127.0.0.1 8080

### Dependencies
- Winsock2 is Required for network communication on Windows. 
- Both the server & client both use the Winsock2 library, which must be linked when compiling (-lws2_32).
