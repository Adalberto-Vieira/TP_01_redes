all:
	gcc -Wall -c commun.c -o bin/commun.o
	gcc -Wall client.c bin/commun.o -o bin/client
	gcc -Wall server.c bin/commun.o -o bin/server