#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include <event.h>

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
		DPRINTF("Client: %d disconnected by remote\n", c->cl_fd);
		bufferevent_free(bufev);
		close(c->cl_fd);
		free(c);
	}

	buf[dlen] = 0;

	DPRINTF("### %s\n", buf);
	
	TAILQ_FOREACH(client, &client_list, cl_entries) {
		if (client != c) {
			bufferevent_write(client->cl_buf_ev, buf, dlen);
		}
	}
}



