#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define LISTENQ 2	//最大上線人數
#define MAXLINE 4096	//client端輸入訊息最大長度
#define FD_SETSIZ 3	//select集合大小

struct client_info	//建立一個結構，用來紀錄使用者資訊
{
	int clientfd;	//使用者檔案子
	char name[20];	//儲存使用者暱稱
	struct sockaddr_in cliaddr;//client端ip位址
};

void broadcast(struct client_info *, int , char *, int);	//將server端收到的訊息，廣播給全部client端
void command(struct client_info *, int, int , char *, int);	//當client輸入 //num 則會顯示目前上站人數
void what_time(struct client_info *, int, char *);		//             //date 則會顯示目前時間日期

void test_broadcast(struct client_info *, int, FILE *, int, int, char *);//用來測試server跟client之間訊息傳送是否正確

int main(int argc, char **argv)
{
	int 			maxi, maxfd, listenfd, connfd, sockfd;
	int			nready; 
	ssize_t			i, n;
	fd_set			rset, allset;
	char			buf[MAXLINE], str[INET_ADDRSTRLEN];
	char			climsg[100], nread;
	socklen_t		clilen;
	struct sockaddr_in	cliaddr, servaddr;
	struct client_info	client[FD_SETSIZ];
	FILE *fp, *ffp = stdin;

	if ((fp=fopen("test//msg.txt","w+"))==NULL) 
		printf("Error in opening %s\n","msg.txt"), exit(1);
	else
		printf("File opening succeed\n");

	//fputs("shaf dsfdsaf dgdasg gdasg", fp);
	//fclose(fp);

	int is_command = 0;

	if (argc != 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}
	
	/*初始socket*/
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family	= AF_INET;
	servaddr.sin_addr.s_addr= htonl(INADDR_ANY);
	servaddr.sin_port	= htons(atoi(argv[1]));

	/*命名socket*/
	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	/*監聽client端連線*/
	listen(listenfd, LISTENQ);

	/*初始一些資料*/
	maxfd = listenfd;
	maxi = -1;
	for (i = 0; i< FD_SETSIZ; i++)
		client[i].clientfd = -1;
	
	/*初始select集合*/
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);

	FD_SET(fileno(ffp), &allset);//將鍵盤輸入檔案子加到集合裡

	/*用迴圈方式一直處理client端要求*/
	while(1)
	{
		rset = allset;
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
	
		/*儲存使用者聊天紀錄並且關掉server*/
		if(FD_ISSET(fileno(ffp), &rset))//當鍵盤輸入時為真
		{
			ioctl(0,FIONREAD,&read);//輸入ctrl+D時會進入下列條件式
			if(nread == 0)
			{
				printf("server shutdown\n");
				fclose(fp);
				exit(1);
			}
		}

		/*如果會進入此迴圈，表示是server端發出的要求，有client要連線*/
		if (FD_ISSET(listenfd, &rset))
		{
			/*接受client端連線*/
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
			
			/*儲存使用者資訊*/
			for (i = 0; i < FD_SETSIZ; i++)
				if (client[i].clientfd < 0)
				{
					client[i].clientfd = connfd;//紀錄client端檔案子
					memcpy(&client[i].cliaddr, &cliaddr, clilen);
					printf("client in: %s:%d [sock:%d]\n",
							inet_ntop(AF_INET, &client[i].cliaddr.sin_addr, str, sizeof(str)),
							ntohs(client[i].cliaddr.sin_port), client[i].clientfd);
					sprintf(climsg, "[client@%s:%d]===>\n",
							inet_ntop(AF_INET, &client[i].cliaddr.sin_addr, str, sizeof(str)),
							ntohs(client[i].cliaddr.sin_port));
					broadcast(client, maxi, climsg, strlen(climsg));//通知所有上線人，該員登入
					break;//當一個client[i]資訊儲存完，馬上離開
				}
#if 0
			if (i == FD_SETSIZ)
			{
				printf("too many clients\n");
				exit(1);
			}
#endif
			if (i == FD_SETSIZ)
			{
				close(connfd);
				goto out_of_num;
			}

			FD_SET(connfd, &allset);
			if (connfd > maxfd)
				maxfd = connfd;
			if (i > maxi)
				maxi = i;

out_of_num:
			if (--nready <= 0)
				continue;
		}

		/*反之，表示由client發的要求，傳送訊息或是離開聊天室*/
		for (i = 0; i <= maxi; i++)
		{
			if ((sockfd = client[i].clientfd) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset))
			{
				/*client端離開聊天室*/
				if ((n = read(sockfd, buf, MAXLINE)) == 0)
				{
					printf("client out: %s:%d [sock:%d]\n",
							inet_ntop(AF_INET, &client[i].cliaddr.sin_addr, str, sizeof(str)),
							ntohs(client[i].cliaddr.sin_port), client[i].clientfd);
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i].clientfd = -1;
					//maxi--;
					sprintf(climsg, "[client@%s:%d]<===\n",
							inet_ntop(AF_INET, &client[i].cliaddr.sin_addr, str, sizeof(str)),
							ntohs(client[i].cliaddr.sin_port));
					broadcast(client, maxi, climsg, strlen(climsg));
				}
				else//client端傳送訊息
				{
#if 0
					for (j = 0; j <= maxi; j++)
					{
						if (client[j].clientfd < 0)
							continue;
						write(client[j].clientfd, buf, n);
					}
#endif
					sprintf(climsg, "[client@%s:%d]:",
							inet_ntop(AF_INET, &client[i].cliaddr.sin_addr, str, sizeof(str)),
							ntohs(client[i].cliaddr.sin_port));
					
					buf[n-1] = '\0';
					if(strcmp(buf,"//num") == 0)//顯示聊天室人數
					{
						is_command = 1;
						test_broadcast(client, maxi, fp, i, is_command, buf);///TEST!!!!
						command(client, i, maxi, buf, n);
					}
					else if(strcmp(buf,"//exit") == 0)//離開聊天室
					{
						printf("client out: %s:%d [sock:%d]\n",
						inet_ntop(AF_INET, &client[i].cliaddr.sin_addr, str, sizeof(str)),
						ntohs(client[i].cliaddr.sin_port), client[i].clientfd);
						close(sockfd);
						FD_CLR(sockfd, &allset);
						client[i].clientfd = -1;
						sprintf(climsg, "[client@%s:%d]<===\n",
							inet_ntop(AF_INET, &client[i].cliaddr.sin_addr, str, sizeof(str)),
							ntohs(client[i].cliaddr.sin_port));
						broadcast(client, maxi, climsg, strlen(climsg));
						is_command = 1;	
					}
					else if(strcmp(buf,"//date") == 0)//顯示目前時間日期
					{
						is_command = 1;//表示client端是傳送指令，將is_command設為1	
						test_broadcast(client, maxi, fp, i, is_command, buf);///TEST!!!!
						what_time(client, i, buf);
					}						
					
					if(is_command==0)//如果is_command為0表示，client端址傳送聊天訊息
					{
						is_command = 0;//清為0
						test_broadcast(client, maxi, fp, i, is_command, buf);//TEST
						buf[n-1] = '\n';
						broadcast(client, maxi, climsg, strlen(climsg));
						broadcast(client, maxi, buf, n);
					}
					
					is_command = 0;//清為0
					
				}
				if (--nready <= 0)
					break;
			}
		}
	}
}

void broadcast(struct client_info *client, int maxindex, char *buf, int n)
{
	int j;
	for (j = 0; j <= maxindex; j++) 
	{
		if (client[j].clientfd < 0)
		{
			continue;
		}
		write(client[j].clientfd, buf, n);
		//fputs(buf, fp);
	}
}

void command(struct client_info *client, int fd, int maxindex, char *buf, int n)
{
	int j, count;
	//char *climsg;

	count = 0;
	for(j = 0; j <= maxindex; j++)
	{
		if(client[j].clientfd >= 0)
			count++;
	}
	sprintf(buf, "there are %d persons in the char room\n", count);
	write(client[fd].clientfd, buf, strlen(buf));
}

void what_time(struct client_info *client, int fd, char *buf)
{	
	time_t timer;
	timer = time(NULL);

	sprintf(buf, "%s", asctime(localtime(&timer)));
	write(client[fd].clientfd, buf, strlen(buf));
}

void test_broadcast(struct client_info *client, int maxindex, FILE *fp, int fd, int is_command, char *buf)
{
	char cTotal[50];
	int j, count;
	count = 0;

	/*一開始都會先計算聊天室內人數*/
	for(j=0 ; j<=maxindex ; j++)
	{
		if(client[j].clientfd >= 0)
			count++;
	}
	snprintf(cTotal, 50, "\nthere are %d persons in the chat room\n", count);
	cTotal[49] = '\n';
	fputs(cTotal, fp);

	/*client端傳送的訊息分為 普通交談 或是 指令*/
	if(is_command == 0)//普通交談===>測試是否所有人都會收到訊息
	{
		snprintf(cTotal, 50, "client[%d] send the >%s< message\n", client[fd].clientfd, buf);
		fputs(cTotal, fp);

		for(j=0 ; j<=maxindex ; j++)
		{
			if(client[j].clientfd>=0)
			{
				snprintf(cTotal, 50, "====>client[%d]:got!\n",client[j].clientfd);
				fputs(cTotal, fp);
			}
		}
	}
	else if(is_command == 1)//指令===>測試是否只有自己收到訊息
	{
		snprintf(cTotal, 50, "client[%d] send the >%s< message\n", client[fd].clientfd, buf);
		fputs(cTotal, fp);

		for(j=0 ; j<=maxindex ; j++)
		{
			if(j == fd)
			{
				//assert(j != fd);
				snprintf(cTotal, 50, "====>client[%d]:got!\n", client[j].clientfd);
			}
			else
			{
				//assert(j == fd);
				snprintf(cTotal, 50, "====>client[%d]:...\n", client[j].clientfd);
			}
			fputs(cTotal, fp);
		}
	}
}
