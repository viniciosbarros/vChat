
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
#include <sqlite3.h>

#include "server.h"
#include "cmd.h"
#include "log.h"

#define DATABASE 	"/var/opt/banco.db"
#define E_WORD			128
#define NOT_AUTHORIZED	5
#define AUTHORIZED 		1

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
	
	if (!(strcmp(cmd,"/connect"))) {
		matche(buf, "%s %s %s", cmd, user, pass);
		log_debug("USER %s is trying to %s: with pass %s\n", 
			user, cmd, pass);
		ret = check_passwd(user, pass);
		strncpy(c->name,user,sizeof(user));
		return (ret);
	}

	if (!(strcmp(cmd,"/quit"))) {
		log_debug("USER is trying to %s", cmd);
		return (CLOSE_CONNECTION);
	}

	return (ret);
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
	sqlite3 *db;
 	sqlite3_stmt *res;
  	const char *tail;
  	const char *tmp_nick;
  	const char *tmp_pass;
  	int ret = NOT_AUTHORIZED;

  	if (sqlite3_open(DATABASE, &db)) {
    	sqlite3_close(db);
    	sqlite3_finalize(res);
    	log_debug("can't open database: %s\n", sqlite3_errmsg(db));
    	return (NOT_AUTHORIZED);
    }

    /* query the database for all users and passwd */
    /* TODO: match only the specific user in the database */
    if (sqlite3_prepare_v2(db, "SELECT NICK,PASSWD FROM USERS;",
    		E_WORD, &res, &tail) != SQLITE_OK) {
    	sqlite3_close(db);
    	sqlite3_finalize(res);
    	log_debug("can't retrieve data: %s\n", sqlite3_errmsg(db));
    	return (NOT_AUTHORIZED);
  	}

  	/* search for all obtained user for the specific one*/
  	while (sqlite3_step(res) == SQLITE_ROW) {
  		tmp_nick = (const char *) sqlite3_column_text(res, 0);
  		tmp_pass = (const char *) sqlite3_column_text(res, 1);
  		if (!strcmp(tmp_nick,user)) {
  			if (!strcmp(tmp_pass,pass)){
  				log_debug("USER: (%s) is authenticated",tmp_nick);
        		ret = AUTHORIZED;
        	}
    	}
  	}

  	sqlite3_finalize(res);
  	sqlite3_close(db);

	log_debug("auth_code_resp(%d) \n", ret);
	return (ret);
}