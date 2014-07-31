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
	$(CC) -o ./bin/vchat  vClient.c
	$(CC) -o ./bin/server server.c

clean:
	/bin/rm -f *.o ./bin/vchat ./bin/server
