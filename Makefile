all: client server

client: 
	gcc ./src/client/client.c -Wall -o ./bin/client

server: 
	gcc ./src/server/server.c -Wall -o ./bin/server

clean:
	rm -f ./bin/client ./bin/server
