#ifndef _SERVER_H_
#define _SERVER_H_

/*

by vinicios - July 2014

Server vChat - servidor Chat messages

*/

#include <sys/queue.h>

void peer_read_cb(struct bufferevent *, void *);

#define DPRINTF(fmt, args...) \
	fprintf(stderr, "vChatServerDaemonDebug: " fmt "\n", ## args);


TAILQ_HEAD(, client) client_list;

struct client {
  int cl_fd;
  struct bufferevent *cl_buf_ev;
  struct event cl_ev_read;
  struct event cl_ev_write;
  TAILQ_ENTRY(client) cl_entries;
};


#endif /* _SERVER_H_ */
