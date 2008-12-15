all: server.o client.o

server: server.o
	gcc -o server server.o
cient: client.o
	gcc -o client client.o
clean:
	rm -f server server.o
	rm -f client client.o
