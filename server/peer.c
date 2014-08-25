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
		bufferevent_free(bufev);
		close(c->fd);
		free(c);

		fprintf(stderr, "Client disconnected\n");
	}

	buf[dlen] = 0;

	fprintf(stderr, "### %s\n", buf);

	TAILQ_FOREACH(client, &client_list, entries) {
		if (client != c) {
			bufferevent_write(client->buf_ev, buf, dlen);
		}
	}
}
