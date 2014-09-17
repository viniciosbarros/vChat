/*
 * Copyright (c) 2014 Vinicios Barros <vinicios.barros@ieee.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifdef __linux__
#define _GNU_SOURCE
#endif /* __linux__ */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

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

#include "log.h"
#include "server.h"

#define CMD_DATABASE 		"/var/opt/banco.db"
#define CMD_WORD			128
#define CMD_NOT_AUTHORIZED	5
#define CMD_AUTHORIZED 		1

static void matche(const char *, const char *, ...);
static int check_passwd(const char *, const char *);

int 
command(const char *buf, struct client *cl)
{
	char cmd[WORD];
	char user[WORD];
	char pass[WORD];
	int ret = 0;

	matche(buf, "%s", cmd);
	log_debug("CMD is: %s\n",cmd);
	
	if (strcmp(cmd,"/connect") == 0) {
		matche(buf, "%s %s %s", cmd, user, pass);
		log_debug("USER %s is trying to %s: with pass %s\n", 
			user, cmd, pass);
		ret = check_passwd(user, pass);
		strncpy(cl->cl_name, user, sizeof(user));
		return (ret);
	}

	if (strcmp(cmd,"/quit") == 0) {
		log_debug("USER is trying to %s", cmd);
		return (CLOSE_CONNECTION);
	}

	return (ret);
}

static void
matche(const char * str, const char * format, ...)
{
	va_list args;
	va_start(args, format);
	vsscanf(str, format, args);
	va_end(args);
}

static int 
check_passwd(const char * user, const char *pass)
{
	sqlite3 *db;
 	sqlite3_stmt *res;
  	const char *tail;
  	const char *tmp_nick;
  	const char *tmp_pass;
  	int ret = CMD_NOT_AUTHORIZED;

  	if (sqlite3_open(CMD_DATABASE, &db)) {
    	sqlite3_close(db); /* never close */
    	log_debug("can't open database: %s\n", sqlite3_errmsg(db));
    	return (CMD_NOT_AUTHORIZED);
    }

    /* query the database for all users and passwd */
    /* TODO: match only the specific user in the database */
    if (sqlite3_prepare_v2(db, "SELECT NICK,PASSWD FROM USERS WHERE NICK like ?;",
    		CMD_WORD, &res, &tail) != SQLITE_OK) {
    	sqlite3_close(db);
    	log_debug("can't retrieve data: %s\n", sqlite3_errmsg(db));
    	return (CMD_NOT_AUTHORIZED);
  	}

  	/* search for all obtained user for the specific one*/
  	while (sqlite3_step(res) == SQLITE_ROW) {
  		tmp_nick = (const char *) sqlite3_column_text(res, 0);
  		tmp_pass = (const char *) sqlite3_column_text(res, 1);
  		if (!strcmp(tmp_nick,user)) {
  			if (!strcmp(tmp_pass,pass)){
  				log_debug("USER: (%s) is authenticated",tmp_nick);
        		ret = CMD_AUTHORIZED;
        	}
    	}
  	}

  	sqlite3_finalize(res);
  	sqlite3_close(db);

	log_debug("auth_code_resp(%d) \n", ret);
	return (ret);
}