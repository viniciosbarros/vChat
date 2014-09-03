/*

by vinicios - July 2014

Server vChat - servidor Chat messages

*/

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <err.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>
#include <event.h>

#include "log.h"
#include "server.h"


#define PORT		(9055)
#define BUFSIZE		(1024)

struct server_ctx sc;

static void usage(void);
static int  set_nonblock(int *);
static void connection_accept(int, short, void *);
static void connect_request(int *);

int
set_nonblock(int *fd)
{
	int flags;

	flags = fcntl(*fd, F_GETFL);
	if (flags == -1)
		return (flags);
	flags |= O_NONBLOCK;
	if (fcntl(*fd, F_SETFL, flags) == -1)
		return (-1);

	return (0);
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

	c->cl_buf_ev = bufferevent_new(c->cl_fd, peer_read_cb, NULL, peer_error_cb, c);
 	bufferevent_enable(c->cl_buf_ev, EV_READ);

	log_debug("Connection established with %s on port %d\n",
	     inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

}

static void
connect_request(int *sockfd)
{
	int yes = 1;
	struct sockaddr_in sin;

	*sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (*sockfd == -1)
		err(1, "socket");

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(PORT);

	if (bind(*sockfd, (struct sockaddr *) &sin, 
		sizeof(sin)) == -1) {
		close(*sockfd);
		err(1, "bind");
	}

	if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
	    sizeof(int)) == -1) {
		close(*sockfd);
		err(2, "setsockopt");
	}

	if (listen(*sockfd, 10) == -1) {
		close(*sockfd);
		err(3, "listen");
	}

	if (set_nonblock(sockfd) == -1) {
		close(*sockfd);
		err(4, "change to non-block");
	}

	event_set(&sc.sc_acceptev, *sockfd, EV_READ | EV_PERSIST,
	    connection_accept, NULL);
	event_add(&sc.sc_acceptev, NULL);

	event_dispatch();

	/* NOTREACHED */
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
	uint32_t opts;

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

	log_debug("vCHAT in the flow");

	event_init();

	TAILQ_INIT(&sc.sc_clist);

	connect_request(&sockfd);

	exit(EXIT_SUCCESS);
}
