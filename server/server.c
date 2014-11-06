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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <err.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <event.h>

#include "log.h"
#include "server.h"


#define PORT		(9055)
#define BUFSIZE		(1024)

struct server_ctx sc;

static void usage(void);
static void set_nonblock(int);
static void connection_accept(int, short, void *);
static int connect_request(void);

static void
set_nonblock(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL);
	if (flags == -1)
		fatal("F_GETFL");
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) == -1)
		fatal("F_SETFL");
}

static void
connection_accept(int fd, short event, void *bula)
{
	socklen_t addrlen;
	int newsockfd;
	struct sockaddr_in sin;
	struct client *c;

	addrlen = sizeof(sin);
	newsockfd = accept(fd, (struct sockaddr *) &sin, &addrlen);
	if (newsockfd == -1) {
		warn("accept");
		return;
	}

	c = calloc(1, sizeof(*c));
	if (c == NULL) {
		warn("calloc");
		close(newsockfd);
		return;
	}

	c->cl_fd = newsockfd;
	TAILQ_INSERT_HEAD(&sc.sc_clist, c, cl_entries);

	c->cl_bufev = bufferevent_new(c->cl_fd, peer_read_cb, NULL, peer_error_cb, c);
 	bufferevent_enable(c->cl_bufev, EV_READ);

	log_debug("Connection established with %s on port %d\n",
	     inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

}

static int
connect_request(void)
{
	int yes = 1;
	int sockfd;
	struct sockaddr_in sin;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		err(1, "socket");

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(PORT);

	if (bind(sockfd, (struct sockaddr *) &sin, 
		sizeof(sin)) == -1) {
		close(sockfd);
		err(1, "bind");
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
	    sizeof(int)) == -1) {
		close(sockfd);
		err(2, "setsockopt");
	}

	if (listen(sockfd, 10) == -1) {
		close(sockfd);
		err(3, "listen");
	}

	set_nonblock(sockfd);

	return (sockfd);
}

static void
usage(void)
{
	extern const char	*__progname;

	fprintf(stderr, "%s: [ -d | -v ]\n"
	    "-d: Debug mode (foreground)\n"
	    "-v: Be verbose\n",
	    __progname);

	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	int	sockfd;
	int	ch;
	uint32_t opts = 0;

#define OPTS_FOREGROUND	(1UL << 0)
#define OPTS_VERBOSE	(1UL << 1)
	while ((ch = getopt(argc, argv, "dv")) != -1) {
		switch (ch) {
		case 'd':
			opts |= OPTS_FOREGROUND;
			break;
		case 'v':
			opts |= OPTS_VERBOSE;
			break;

		default:
			usage();
			/* NOTREACHED */
		}
	}

	if ((opts & OPTS_FOREGROUND) == 0) {
		log_init(0);
		daemon(0, 0);
	} else
		log_init(1);

	if (opts & OPTS_VERBOSE)
		log_verbose(1);

	log_debug("vCHAT in the even flow");

	event_init();

	TAILQ_INIT(&sc.sc_clist);

	sockfd = connect_request();

	event_set(&sc.sc_acceptev, sockfd, EV_READ | EV_PERSIST,
	    connection_accept, NULL);
	event_add(&sc.sc_acceptev, NULL);

	event_dispatch();

	exit(EXIT_SUCCESS);
}
