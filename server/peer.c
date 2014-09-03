#include <errno.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include <event.h>

#include "log.h"
#include "server.h"

void
peer_read_cb(struct bufferevent *bufev, void *bula)
{
	struct client *c = bula;
	struct client *client;
	size_t dlen;
	char buf[512];

	dlen = bufferevent_read(bufev, buf, sizeof(buf));
	if (dlen == 0) {
		log_debug("Client: %d disconnected by remote\n", c->cl_fd);
		bufferevent_disable(bufev, EV_READ);
		bufferevent_free(bufev);
		close(c->cl_fd);
		TAILQ_REMOVE(&sc.sc_clist, c, cl_entries);
		free(c);
		return;
	}

	buf[dlen] = 0;
	
	TAILQ_FOREACH(client, &sc.sc_clist, cl_entries) {
		if (client == c)
			continue;

		bufferevent_write(client->cl_buf_ev, buf, dlen);
	}
}

void
peer_error_cb(struct bufferevent *bufev, short what, void *bula)
{
	struct client *c = bula;

	log_debug("Client: %d disconnected by remote\n", c->cl_fd);
	bufferevent_disable(bufev, EV_READ);
	bufferevent_free(bufev);
	close(c->cl_fd);

	TAILQ_REMOVE(&sc.sc_clist, c, cl_entries);
	free(c);
}
