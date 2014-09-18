/*
 * Copyright (c) 2014 Vinicios Barros <vinicios.barros@ieee.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <event.h>

#include "log.h"
#include "server.h"


static void remove_client(struct client *);
static void response(struct client *, const char *, size_t, int);

void
peer_read_cb(struct bufferevent *bufev, void *bula)
{
	struct client *cl = bula;
	size_t dlen;
	char buf[512];
	int ret = 0;

	dlen = bufferevent_read(bufev, buf, sizeof(buf));
	if (dlen == 0) {
		log_debug("Client: %d disconnected by remote\n", cl->cl_fd);
		remove_client(cl);
		free(cl);
		return;
	}

	buf[dlen] = 0;

	log_debug("MSG: %s\n", buf);
	if (buf[0] == '/') {
		log_debug("string is a CMD: %s\n", buf);
		ret = command(buf, cl);
		if (ret == 1)
			strncpy(buf, "ok", dlen);
	}

	response(cl, buf, dlen, ret);

}

void
peer_error_cb(struct bufferevent *bufev, short what, void *bula)
{
	struct client *cl = bula;

	log_debug("Client: %d disconnected by remote\n", cl->cl_fd);
	remove_client(cl);

}

static void 
remove_client(struct client *cl)
{
	bufferevent_disable(cl->cl_bufev, EV_READ);
	bufferevent_free(cl->cl_bufev);

	close(cl->cl_fd);

	TAILQ_REMOVE(&sc.sc_clist, cl, cl_entries);

	free(cl);

}

static void 
response(struct client *cl, const char *buf, size_t dlen, int dest)
{

	struct client *cln;

	/* In case buf was a command response*/
	if (dest) {
		bufferevent_write(cl->cl_bufev, buf, dlen);

		return;		
	}

	/* In case buf was a message to all*/	
	TAILQ_FOREACH(cln, &sc.sc_clist, cl_entries) {
		if (cln == cl)
			continue;

		bufferevent_write(cln->cl_bufev, buf, dlen);
	}

}