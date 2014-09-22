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

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <err.h>

#define BUFSIZE 2048
#define WORD 128
#define SERVER_IP "127.0.0.1"	//Local Server Address
#define PORT 9055

struct User
{
	int id;
	char name[WORD];
	char passwd[WORD];
	char room[WORD];
};

int auth(struct User, int);
int cmd_prompt(struct User, char *, int);
void close_session(struct User,int);
void send_recv(int, int,struct User);
int connect_request(int*, struct sockaddr_in*);

int
auth(struct User user, int sockfd)
{
	char pass[WORD];
	char send_buf[BUFSIZE];
	char recv_buf[BUFSIZE];
	int nbyte_recvd;
	int sn_bytes;

	printf("%s vChat(%s) password: ", user.name, SERVER_IP);
	system("stty -echo");
	gets(pass);
	system("stty echo");
	strncpy(user.passwd, pass, sizeof(pass));

	sn_bytes = sprintf(send_buf, "/connect %s %s %s", user.name, user.passwd, user.room);	
	send(sockfd, send_buf, sn_bytes, 0);
	
	nbyte_recvd = recv(sockfd, recv_buf, BUFSIZE, 0);
	recv_buf[nbyte_recvd] = '\0';

	if (strcmp(recv_buf,"ok")) {
		printf("\nERROR: can't authenticate code(%s)\n", recv_buf);
		exit (1);
	}

	printf("\nOK: %s\n", recv_buf);

	fflush(stdout);

	return (0);
}

int
cmd_prompt(struct User user, char * send_buf, int sockfd)
{
	/*printf("client-quited\n");
    strncpy(send_buf,user.name,sizeof(user.name));*/
    send(sockfd, send_buf, strlen(send_buf), 0);
	return (0);
}

void
close_session(struct User user, int sockfd)
{
	char send_buf[BUFSIZE];

	printf("client will quit\n");
	sprintf(send_buf, "%s : User %s has left the room %s",
	    user.room, user.name, user.room);

	send (sockfd, send_buf, strlen(send_buf), 0);

}

void
send_recv(int i, int sockfd, struct User user)
{
	char send_buf[BUFSIZE];
	char message_str[BUFSIZE - 512];
	char recv_buf[BUFSIZE];
	char room[WORD];
	int nbyte_recvd;
	char stime[WORD];
	time_t rawtime;
	struct tm *timeinfo;
	int sn_bytes;

	if (i == 0) {
		fgets(message_str, BUFSIZE, stdin);
		if (!(strcmp(message_str, "/quit\n"))) {
			close_session(user, sockfd);
			exit(0);
		} else if (message_str[0] == '/') {
			cmd_prompt(user, message_str, sockfd);
		} else {

			time(&rawtime);
			timeinfo = localtime(&rawtime);
			strftime(stime, sizeof(stime), "%H:%M:%S", timeinfo);

			sn_bytes = sprintf(send_buf, "%s room: (%s) (%s): %s",
			    user.room, user.name, stime, message_str);
			send_buf[sn_bytes] = '\0';
			send(sockfd, send_buf, sn_bytes, 0);
		}
	} else {
		nbyte_recvd = recv(sockfd, recv_buf, BUFSIZE, 0);
		recv_buf[nbyte_recvd] = '\0';

		sscanf(recv_buf, "%s ", room);

		if (!(strcmp(room, user.room)))
			printf("%s\n", recv_buf);

		fflush (stdout);
	}
}

int
connect_request(int *sockfd, struct sockaddr_in *server_addr)
{
	if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket");
		return (1);
	}

	server_addr->sin_family = AF_INET;
	server_addr->sin_port = htons(PORT);
	server_addr->sin_addr.s_addr = inet_addr(SERVER_IP);
	memset(server_addr->sin_zero, '\0', sizeof server_addr->sin_zero);

	if (connect
	    (*sockfd, (struct sockaddr *) server_addr,
		sizeof(struct sockaddr)) < 0) {
		perror("connect");
		close(*sockfd);
		return (1);
	}

	return (0);
}

int
main(int argc, char *argv[])
{
	int sockfd, fdmax, i, ret;
	struct sockaddr_in server_addr;
	fd_set master, read_fds;
	struct User user;
	strncpy(user.room, "Default", 8);

	if (argc < 2) {
		getlogin_r(user.name, sizeof(user.name));
		if (!strcmp(user.name,""))
			printf
			    ("\nNO user informed: Using system's username: %s\n",
			    user.name);
	} else if (argc == 3 || argc > 3) {
		strncpy(user.room, argv[2], (sizeof(argv[2])+1));
		strncpy(user.name, argv[1], (sizeof(argv[1])+1));
	} else
		strncpy(user.name, argv[1], (sizeof(argv[1])+1));

	

	ret = connect_request(&sockfd, &server_addr);
	if (ret)
		exit (1);


	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(0, &master);
	FD_SET(sockfd, &master);
	fdmax = sockfd + 1;

	ret = auth(user, sockfd);
	if (ret) {
		printf("\nERROR: authentication error\n");
		return (401);
	}

	printf("\n\nWellcome to Room #(%s) \n\n", user.room);
	fflush(stdout);

	while (1) {
		read_fds = master;
		if (select(fdmax, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit (4);
		}

		for (i = 0; i <= fdmax; i++)
			if (FD_ISSET(i, &read_fds))
				send_recv(i, sockfd, user);
	}
	close (sockfd);
	return 0;
}
