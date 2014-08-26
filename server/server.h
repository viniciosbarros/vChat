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
  struct event ev_read;
  struct event ev_write;
  TAILQ_ENTRY(client) entries;
};

#endif /* _SERVER_H_ */
