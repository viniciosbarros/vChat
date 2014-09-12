#ifndef _SERVER_H_
#define _SERVER_H_

/*

by vinicios - July 2014

Server vChat - servidor Chat messages

*/

#include <sys/queue.h>

#define WORD	32

extern TAILQ_HEAD(CLIST, client) client_list;

struct client {
	TAILQ_ENTRY(client) cl_entries;

	int		cl_fd;
	struct		bufferevent *cl_buf_ev;
	struct		event cl_ev_read;
	struct		event cl_ev_write;

	char name[WORD];
	char passwd[WORD];
};

struct server_ctx {
	TAILQ_HEAD(, client) sc_clist;

	struct		event sc_acceptev;
};

extern struct server_ctx sc;

void peer_read_cb(struct bufferevent *, void *);
void peer_error_cb(struct bufferevent *, short, void *);

#endif /* _SERVER_H_ */
