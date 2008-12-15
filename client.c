#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAXLINE 4096
#define max(a,b)	((a) > (b) ? (a) : (b))

void client_chat(FILE *fp, int sockfd)
{
	int 	maxfdp1, stdineof, i;
	fd_set	rset;
	char	buf[MAXLINE];
	int 	n;
	char    nickname[255];
	char    lines[1024];

	stdineof = 0;
	FD_ZERO(&rset);
	printf("what's your name?");
    	scanf("%s",nickname);
	for ( ; ; ) 
	{
		if (stdineof == 0)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfdp1 = max(fileno(fp), sockfd) + 1;
		select(maxfdp1, &rset, NULL, NULL, NULL);

		
		if (FD_ISSET(sockfd, &rset)) 
		{
			if ((n = read(sockfd, buf, MAXLINE)) == 0) 
			{ 
				if (stdineof == 1)
					return;
				else 
				{
					printf("client_chat: server terminated...\n");
					exit(1);
				}
			}
			write(fileno(stdout), buf, n);
		}

		if (FD_ISSET(fileno(fp), &rset)) 
		{
			if ((n = read(fileno(fp), buf, MAXLINE)) == 0) 
			{
				stdineof = 1;
				shutdown(sockfd, SHUT_WR);
				FD_CLR(fileno(fp), &rset);
				continue;
			}
			write(sockfd, buf, n);
		}
	}
}

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in servaddr;

	if (argc != 3) 
	{
		printf("Usage: %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	
	client_chat(stdin, sockfd);

	exit(0);
}
