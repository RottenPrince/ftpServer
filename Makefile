CC = gcc
CFLAGS = -Wall -Wextra

all: client server 

client: ./src/client/client.c
	$(CC) $(CFLAGS) $^ -o ./bin/client

server: ./src/server/server.c 
	$(CC) $(CFLAGS) $^ -o ./bin/server

clean:
	rm -f ./bin/client
	rm -f ./bin/server
