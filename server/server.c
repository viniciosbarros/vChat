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

#include "server.h"


#define PORT 	9055
#define BUFSIZE 1024

static struct event ev_accept;
static struct event_base *ev_base;

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
	if (fcntl(*fd, F_SETFL, flags) < 0)
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
	TAILQ_INSERT_HEAD(&client_list, c, cl_entries);

	c->cl_buf_ev = bufferevent_socket_new(ev_base, newsockfd, 0);
	bufferevent_setcb(c->cl_buf_ev, peer_read_cb, NULL, NULL, c);

	bufferevent_enable(c->cl_buf_ev, EV_READ);

	DPRINTF("Connection established with %s on port %d\n",
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

	if(set_nonblock(sockfd) == -1){
		close(*sockfd);
		err(4, "change to non-block");
	}

	event_set(&ev_accept, *sockfd, EV_READ|EV_PERSIST, 
		connection_accept, NULL);
	event_add(&ev_accept, NULL);

	event_dispatch();

	DPRINTF("Listening clients on port %d", PORT);
}

int
main(int argc, char *argv[])
{
	int sockfd;

	daemon(0, 1);
	openlog("vChatServer", LOG_PID, LOG_DAEMON);

	DPRINTF("vCHAT in the flow");

	event_init();

	TAILQ_INIT(&client_list);

	connect_request(&sockfd);

	DPRINTF("vCHAT will be looped");

	exit(EXIT_SUCCESS);
}
