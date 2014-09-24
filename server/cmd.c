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

#include "log.h"
#include "server.h"

#define CALL_SQLITE(f)											\
	{															\
		int i;													\
		i = sqlite3_ ## f;										\
		if (i != SQLITE_OK) {									\
			log_debug ("%s failed with status %d: %s\n", #f, i, \
				sqlite3_errmsg(passwd_db));						\
				return (CMD_NOT_AUTHORIZED);					\
		}														\
	}															\


static sqlite3 *passwd_db = NULL;

static void matche(const char *, const char *, ...);
static int check_passwd(const char *, const char *);
static sqlite3 * open_database(void);
static int private_message(const char *);

int 
command(const char *buf, struct client *cl, char * cmd)
{
	char user[WORD];
	char pass[WORD];
	char room[WORD];
	int ret = 0;

	matche(buf, "%s", cmd);
	log_debug("CMD is: %s\n",cmd);
	
	if (strcmp(cmd,"/connect") == 0) {
		matche(buf, "%s %s %s %s", cmd, user, pass, room);
		log_debug("USER %s is trying to %s: with pass %s\n", 
			user, cmd, pass);
		ret = check_passwd(user, pass);
		strncpy(cl->cl_name, user, sizeof(user));
		strncpy(cl->cl_room, room, sizeof(room));
		return (ret);
	}

	if (strcmp(cmd,"/quit") == 0) {
		log_debug("USER is trying to %s", cmd);
		return (CLOSE_CONNECTION);
	}

	if (strcmp(cmd,"/secret") == 0) {
		matche(buf, "%s %s", cmd, user);
		log_debug("USER %s is trying to say a secret to %s\n", 
			cl->cl_name, user);
		ret = private_message(buf);
		strncpy(cmd, user, sizeof(user));
		return (ret);
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
private_message(const char * buf)
{
	return (2);
}

static int 
check_passwd(const char * user, const char *pass)
{
	sqlite3_stmt *res;
	const char *tail;
	const char *tmp_pass;
	int ret = CMD_NOT_AUTHORIZED;
	char *sql;
	sql = "SELECT * FROM USERS WHERE NICK LIKE ?";


	passwd_db = open_database();

	CALL_SQLITE (prepare_v2(passwd_db, sql, strlen (sql),
		&res, &tail));

	CALL_SQLITE (bind_text(res, 1, user, strlen (user), 0));

	while (sqlite3_step(res) == SQLITE_ROW) {
		tmp_pass = (const char *) sqlite3_column_text(res, 1);
		if (strcmp(tmp_pass,pass) == 0) {
			log_debug("USER: (%s) is authenticated",user);
			ret = CMD_AUTHORIZED;
		}
	}

	sqlite3_reset(res);

	log_debug("auth_code_resp(%d) \n", ret);
	return (ret);
}

static sqlite3 *
open_database(void)
{

	if (passwd_db != NULL)
		return (passwd_db);
	
	if (sqlite3_open(CMD_DATABASE, &passwd_db))
		log_debug("can't open database: %s\n",
			sqlite3_errmsg(passwd_db));

	return (passwd_db);
}