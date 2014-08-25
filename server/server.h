#ifndef _SERVER_H_
#define _SERVER_H_

/*

by vinicios - July 2014

Server vChat - servidor Chat messages

*/

#include <sys/queue.h>

struct client {
  int fd;
  struct bufferevent *buf_ev;
  TAILQ_ENTRY(client) entries;
};

void peer_read_cb(struct bufferevent *, void *);

#endif /* _SERVER_H_ */