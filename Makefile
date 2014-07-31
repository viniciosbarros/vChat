#
##
#
## dummy makefile - for vChat
#
##


CC=gcc
CFLAGS=
DEPS= 
OBJ=  

all:
	$(CC) -o vchat vClient.c
	$(CC) -o server server.c

clean:
	/bin/rm -f *.o pchat server
