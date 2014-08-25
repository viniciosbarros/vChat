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
#include <syslog.h>
#include <signal.h>

/* Libevent. */
#include <event.h>
#include "server.h"

#define PORT 9055
#define BUFSIZE 1024

static struct event ev_accept;

TAILQ_HEAD(, client) client_list;
TAILQ_INIT(&client_list);

#define DPRINTF(fmt, args...) \
	fprintf(stderr, "D: " fmt "\n", ## args);

void send_to_all(struct bufferevent *, void *);
void send_recv(int, fd_set *, int, int);
static void connection_accept(int, short, void *);
static void connect_request(int *);

void
send_to_all(int j, int i, int sockfd, int nbytes_recvd, char *recv_buf,
    fd_set * master)
/*send_to_all(struct bufferevent *buf_ev, void *arg)*/
{
	if (FD_ISSET(j, master)) {
		if (j != sockfd && j != i) {
			if (send(j, recv_buf, nbytes_recvd, 0) == -1) {
				perror("send");
			}
		}
	}
}

void
send_recv(int i, fd_set * master, int sockfd, int fdmax)
{
	int nbytes_recvd, j;
	char recv_buf[BUFSIZE];

	nbytes_recvd = recv(i, recv_buf, BUFSIZE, 0);
	switch (nbytes_recvd) {
	case 0:
		DPRINTF("socket %d was unexpected terminated by remote\n", i);
		close(i);
		FD_CLR(i, master);
	case -1:
		DPRINTF("Error: socket %d will be terminated\n", i);
		close(i);
		FD_CLR(i, master);
	default:
		for (j = 0; j <= fdmax; j++) {
			/*send_to_all(j, i, sockfd, nbytes_recvd, recv_buf,
			    master, void *void);*/
		}
	}
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

	c->fd = newsockfd;
	TAILQ_INSERT_HEAD(&client_list, c, entries);

	c->buf_ev = bufferevent_new(c->fd, peer_read_cb, NULL, NULL, c);
	bufferevent_enable(c->buf_ev, EV_READ);

	DPRINTF("Connection established with %s on port %d \n",
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
	sin.sin_port = htons(PORT);
	sin.sin_addr.s_addr = INADDR_ANY;

	if (setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
	    sizeof(int)) == -1) {
		close(*sockfd);
		err(1, "setsockopt");
	}

	if (bind(*sockfd, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
		close(*sockfd);
		err(1, "bind");
	}

	if (listen(*sockfd, 10) == -1) {
		close(*sockfd);
		err(1, "listen");
	}

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

	connect_request(&sockfd);

	event_set(&ev_accept, sockfd, EV_READ, connection_accept, NULL);
	event_add(&ev_accept, NULL);

	DPRINTF("vCHAT will be looped");

	event_dispatch();

	exit(EXIT_SUCCESS);
}
