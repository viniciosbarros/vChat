/*
	Client vChat
	By vinicios Jul 2014
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 2048
#define WORD 32
#define SERVER_IP "192.168.201.160"	//Local Server Address
#define PORT 9055

struct User
{
	int id;
	char name[WORD];
	char passwd[WORD];
	char room[WORD];
};

int
auth(struct User user)
{
	char pass[WORD];

	printf("%s vChat(%s) password: ", user.name, SERVER_IP);
	system("stty -echo");
	scanf("%s", &pass);
	system("stty echo");
	strcpy(user.passwd, pass);

	return (0);
}

int
cmd_prompt(struct User user)
{
	printf("client-quited\n");
//      strcpy(send_buf,user.name);
//      send(sockfd, send_buf, strlen(send_buf), 0);
	return 0;
}

void
close_session(struct User user, int sockfd)
{
	char send_buf[BUFSIZE];

	printf("client will quit\n");
	sprintf(send_buf, "%s : User %s has left the room %s",
	    user.room, user.name, user.room);

	send(sockfd, send_buf, strlen(send_buf), 0);

}

void
send_recv(int i, int sockfd, struct User user)
{
	char send_buf[BUFSIZE];
	char message_str[BUFSIZE - 512];
	char recv_buf[BUFSIZE];
	char room[WORD];
	int nbyte_recvd;
	time_t ltime;
	struct tm *result;
	char stime[WORD];
	time_t rawtime;
	struct tm *timeinfo;

	if (i == 0) {
		fgets(message_str, BUFSIZE, stdin);
		if (!(strcmp(message_str, "/quit\n"))) {
			close_session(user, sockfd);
			exit(0);
		} else if (!(strcmp(message_str, "/cmd\n"))) {
			cmd_prompt(user);
			exit(0);
		} else {

			time(&rawtime);
			timeinfo = localtime(&rawtime);
			strftime(stime, sizeof(stime), "%H:%M:%S", timeinfo);

			sprintf(send_buf, "%s room: (%s) (%s): %s",
			    user.room, user.name, stime, message_str);
			send(sockfd, send_buf, strlen(send_buf), 0);
		}
	} else {
		nbyte_recvd = recv(sockfd, recv_buf, BUFSIZE, 0);
		recv_buf[nbyte_recvd] = '\0';

		sscanf(recv_buf, "%s ", room);

		if (!(strcmp(room, user.room)))
			printf("%s\n", recv_buf);

		fflush(stdout);
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
	strcpy(user.room, "Default");

	if (argc < 2) {
		getlogin_r(user.name, sizeof(user.name));
		if (user.name != "")
			printf
			    ("\nNO user informed: Using system's username: %s\n",
			    user.name);
	} else if (argc == 3 || argc > 3) {
		strcpy(user.room, argv[2]);
		strcpy(user.name, argv[1]);
	} else
		strcpy(user.name, argv[1]);

	ret = auth(user);
	if (ret) {
		printf("\nERROR: authentication error\n");
		return (401);
	}

	printf("\n\nWellcome to Room #(%s) \n\n", user.room);
	fflush(stdout);

	ret = connect_request(&sockfd, &server_addr);
	if (ret)
		exit(1);

	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(0, &master);
	FD_SET(sockfd, &master);
	fdmax = sockfd + 1;

	while (1) {
		//printf("$> ");
		read_fds = master;
		if (select(fdmax, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}

		for (i = 0; i <= fdmax; i++)
			if (FD_ISSET(i, &read_fds))
				send_recv(i, sockfd, user);
	}
	close(sockfd);
	return 0;
}
