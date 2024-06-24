all:
	gcc -std=c11 -pthread -Wall -c commun.c -o bin/commun.o
	gcc -std=c11 -pthread -Wall client.c bin/commun.o -o bin/client
	gcc -std=c11 -pthread -Wall server.c bin/commun.o -o bin/server -lm