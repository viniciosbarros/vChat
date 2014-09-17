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

#ifndef _SERVER_H_
#define _SERVER_H_

/*

by vinicios - July 2014

Server vChat - servidor Chat messages

*/

#include <sys/queue.h>

#define WORD	32
#define CLOSE_CONNECTION 5

struct client {
	TAILQ_ENTRY(client) cl_entries;

	int		cl_fd;
	struct		bufferevent *cl_bufev;
	struct		event cl_readev;
	struct		event cl_writeev;

	char cl_name[WORD];
	char cl_passwd[WORD];
};

struct server_ctx {
	TAILQ_HEAD(, client) sc_clist;

	struct		event sc_acceptev;
};

extern struct server_ctx sc;

void peer_read_cb(struct bufferevent *, void *);
void peer_error_cb(struct bufferevent *, short, void *);

int	command(const char *, struct client *);

#endif /* _SERVER_H_ */
