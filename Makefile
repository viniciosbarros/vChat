#
##
#
## dummy makefile - for vChat
#
##


CC=gcc
CFLAGS=-levent 
DEPS= 
OBJ=  

all:
	$(CC) $(CFLAGS)-o ./bin/vchat  vClient.c
	$(CC) $(CFLAGS) -o ./bin/server server.c

clean:
	/bin/rm -f *.o ./bin/vchat ./bin/server
