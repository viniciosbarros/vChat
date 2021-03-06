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
static void response(struct client *, const char *, size_t, int, char *);

void
peer_read_cb(struct bufferevent *bufev, void *bula)
{
	struct client *cl = bula;
	size_t dlen;
	char buf[PEER_MSG];
	char handler[WORD];
	int ret = 0;

	dlen = bufferevent_read(bufev, buf, sizeof(buf));
	if (dlen == 0) {
		log_debug("Client: %d disconnected by remote\n", cl->cl_fd);
		remove_client(cl);
		return;
	}

	buf[dlen] = 0;

	if (buf[0] == '/') {
		log_debug("command received: %s\n", buf);
		ret = command(buf, cl, handler);
		if (ret == CMD_AUTHORIZED && (strcmp(handler,"/connect")==0))
			strncpy(buf, "ok", dlen);
		if (ret == CMD_NOT_AUTHORIZED && (strcmp(handler,"/connect")==0)) {
			strncpy(buf, "nok", dlen);
			ret = CMD_RESPONSE;
		}
	}

	response(cl, buf, dlen, ret, handler);

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
response(struct client *cl, const char *buf, size_t dlen, 
	int dest, char *dst_name)
{

	struct client *cln;
	char snd_buf[PEER_MSG];
	size_t sn_size;


	/* In case buf was a command response */
	if (dest == CMD_RESPONSE) {
		bufferevent_write(cl->cl_bufev, buf, dlen);

		return;		
	}

	/* In case buf was a private message */
	if (dest == CMD_DIRECT_RESPONSE){
		TAILQ_FOREACH(cln, &sc.sc_clist, cl_entries) {
			if (strcmp(cln->cl_name, dst_name) == 0) {
				sn_size = snprintf(snd_buf, PEER_MSG, "%s room: (%s) %s",
					cln->cl_room, cl->cl_name, buf);
				bufferevent_write(cln->cl_bufev, snd_buf, sn_size);

				return;
			}
		}
	}

	/* In case buf was a message to all (regular flow) */
	TAILQ_FOREACH(cln, &sc.sc_clist, cl_entries) {
		if (cln == cl)
			continue;

		bufferevent_write(cln->cl_bufev, buf, dlen);
	}

}