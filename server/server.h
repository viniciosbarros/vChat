#ifndef _SERVER_H_
#define _SERVER_H_

/*

by vinicios - July 2014

Server vChat - servidor Chat messages

*/

#include <sys/queue.h>

struct client {
  int cl_fd;
  struct bufferevent *cl_buf_ev;
  struct event cl_ev_read;
  struct event cl_ev_write;
  TAILQ_ENTRY(client) cl_entries;
};

#endif /* _SERVER_H_ */
