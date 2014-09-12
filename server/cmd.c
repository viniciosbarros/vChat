
#ifdef __linux__
#define _GNU_SOURCE
#endif /* __linux__ */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#ifdef __linux__
#include <time.h>
#endif /* __linux__ */
#include <unistd.h>

#include <event.h>

#include "server.h"
#include "cmd.h"
#include "log.h"

static void matche(const char *, const char *, ...);
static int check_passwd(char *, char *);

int command(const char * buf, struct client *c)
{
	char cmd[WORD];
	char user[WORD];
	char pass[WORD];
	int ret = 0;

	matche(buf, "%s", cmd);
	log_debug("CMD is: %s\n",cmd);
	
	if (!(strcmp(cmd,"/connect"))){
		matche(buf, "%s %s %s", cmd, user, pass);
		log_debug("USER %s is trying to %s: with pass %s\n", 
			user, cmd, pass);
		ret = check_passwd(user, pass);
		return(ret);
	}

	if (!(strcmp(cmd,"/quit"))){
		log_debug("USER is trying to %s", cmd);
		ret = CLOSE_CONNECTION;
		return(ret);
	}

	return(ret);
}

void matche(const char * str, const char * format, ...)
{
	va_list args;
	va_start (args, format);
	vsscanf (str, format, args);
	va_end (args);
}

int check_passwd(char * user, char *pass)
{
	int ret = 1;
	log_debug("checking: %s %s : vou responder %d \n",user, pass, ret);
	return(ret);
}