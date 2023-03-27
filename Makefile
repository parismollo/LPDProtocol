all:
	gcc -Wall client.c -o client
	gcc -Wall server.c -o server

clean:
	rm -rf server
	rm -rf client