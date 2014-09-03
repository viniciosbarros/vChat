CFLAGS	+= -I. -g -O0
CFLAGS	+= -Wall -Wpointer-arith
CFLAGS	+= -Wstrict-prototypes -Wmissing-prototypes
CFLAGS	+= -Wmissing-declarations -Wshadow
CFLAGS	+= -Wpointer-arith -Wcast-qual
CFLAGS	+= -Wsign-compare

LDFLAGS	+= -levent

PROG	= server

SRCS	= log.c peer.c server.c

.include <bsd.prog.mk>
