#include <errno.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>

#include <event.h>

#include "log.h"
#include "server.h"
#include "cmd.h"

static void remove_client(struct bufferevent *, struct client*);
static void response(struct bufferevent *, struct client*, 
	char *, size_t, int);

void
peer_read_cb(struct bufferevent *bufev, void *bula)
{
	struct client *c = bula;
	size_t dlen;
	char buf[512];
	int ret = 0;

	dlen = bufferevent_read(bufev, buf, sizeof(buf));
	if (dlen == 0) {
		log_debug("Client: %d disconnected by remote\n", c->cl_fd);
		remove_client(bufev, c);
		free(c);
		return;
	}

	buf[dlen] = 0;

	log_debug("MSG: %s\n",buf);
	if (buf[0] == '/') {
		log_debug("string is a CMD: %s\n",buf);
		ret = command(buf, c);
		if(ret == 1)
			strncpy(buf,"ok",dlen);
	}

	response(bufev, c, buf, dlen, ret);

}

void
peer_error_cb(struct bufferevent *bufev, short what, void *bula)
{
	struct client *c = bula;

	log_debug("Client: %d disconnected by remote\n", c->cl_fd);
	remove_client(bufev, c);
	free(c);
}

void remove_client(struct bufferevent *bufev, struct client *c)
{
	bufferevent_disable(bufev, EV_READ);
	bufferevent_free(bufev);
	close(c->cl_fd);
	TAILQ_REMOVE(&sc.sc_clist, c, cl_entries);
	log_debug("Client: %d hit the ground\n", c->name);
	return;
}

void response(struct bufferevent *bufev, 
			struct client *c,  char buf[], 
			size_t dlen, int dest)
{

	struct client *client;

	/* In case buf was a command response*/
	if(dest){
		bufferevent_write(c->cl_buf_ev, buf, dlen);
		return;		
	}

	/* In case buf was a message to all*/	
	TAILQ_FOREACH(client, &sc.sc_clist, cl_entries) {
		if (client == c)
			continue;

		bufferevent_write(client->cl_buf_ev, buf, dlen);
	}

}