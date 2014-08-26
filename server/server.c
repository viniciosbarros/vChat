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

/* Libevent. */
#include <event.h>
#include "server.h"

#define PORT 9055
#define BUFSIZE 1024

static struct event ev_accept;
static struct event_base *ev_base;

TAILQ_HEAD(, client) client_list;

#define DPRINTF(fmt, args...) \
	fprintf(stderr, "D: " fmt "\n", ## args);

void send_to_all(struct bufferevent *, void *);
/*void send_to_all(int, int, int, int, char *, fd_set*);*/
void send_recv(int, fd_set *, int, int);
static void connection_accept(int, short, void *);
static void connect_request(int *);
int changeTononblock(int *);


int 
changeTononblock(int *fd)
{
	int flags;

	flags = fcntl(*fd, F_GETFL);
	if (flags < 0)
		return flags;
	flags |= O_NONBLOCK;
	if (fcntl(*fd, F_SETFL, flags) < 0)
		return -1;

	return 0;
}


void
/*send_to_all(int j, int i, int sockfd, int nbytes_recvd, char *recv_buf,
    fd_set * master)*/
send_to_all(struct bufferevent *buf_ev, void *arg)
{
	/*if (FD_ISSET(j, master)) {
		if (j != sockfd && j != i) {
			if (send(j, recv_buf, nbytes_recvd, 0) == -1) {
				perror("send");
			}
		}
	}*/

	size_t n;
	struct client *c = arg;
	struct client *client;
	uint8_t data[BUFSIZE];

	while(1){
		n = bufferevent_read(buf_ev, data, sizeof(data));
		if (n <= 0){
			bufferevent_free(buf_ev);
			close(c->fd);
			free(c);
		}

		TAILQ_FOREACH(client, &client_list, entries) {
			if (client != c) {
				bufferevent_write(client->buf_ev, data, n);
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

void
peer_read_cb(struct bufferevent *bufev, void *bula)
{
	struct client *c = bula;
	struct client *client;
	size_t dlen;
	char buf[512];

	dlen = bufferevent_read(bufev, buf, sizeof(buf));
	if (dlen == 0) {
		bufferevent_free(bufev);
		close(c->fd);
		free(c);

		fprintf(stderr, "Client disconnected\n");
	}

	buf[dlen] = 0;

	fprintf(stderr, "### %s\n", buf);

	TAILQ_FOREACH(client, &client_list, entries) {
		/* if (client != c) { */ /*Send to all even the sender*/
			bufferevent_write(client->buf_ev, buf, dlen);
		/*}*/
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

	c->buf_ev = bufferevent_socket_new(ev_base, newsockfd, 0);
	bufferevent_setcb(c->buf_ev, peer_read_cb, NULL, NULL, c);

	/*c->buf_ev = bufferevent_new(c->fd, peer_read_cb, NULL, NULL, c);*/
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
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(PORT);

	if (bind(*sockfd, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
		close(*sockfd);
		err(5, "bind");
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

	if(changeTononblock(sockfd) < 0){
		close(*sockfd);
		err(4, "change to non-block");	
	}

	event_set(&ev_accept, *sockfd, EV_READ|EV_PERSIST, connection_accept, NULL);
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
