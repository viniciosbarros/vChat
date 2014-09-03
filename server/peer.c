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
		TAILQ_REMOVE(&client_list, c, cl_entries);
		free(c);
		return;
	}

	buf[dlen] = 0;

	DPRINTF("=> %s\n", buf);
	
	TAILQ_FOREACH(client, &client_list, cl_entries) {
		if (client != c)
			bufferevent_write(client->cl_buf_ev, buf, dlen);
	}
}



