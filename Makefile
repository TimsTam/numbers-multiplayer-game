CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lws2_32 -lpthread

# Name of the output executable
TARGET = game-server

# List of source files
SRC = game-server.c

# Rule to compile the program
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Clean up the build files
clean:
	rm -f $(TARGET)
