all: client server

client: client.c 
	gcc ./src/client.c -Wall -o ./bin/client

server: server.c
	gcc ./src/server.c -Wall -o ./bin/server

clean:
	rm -f client server
