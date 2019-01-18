#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <pthread.h>

typedef struct llist
{
	int fileDesc;
	struct llist *next;
}llist;

typedef struct socketStruct{
	int socketFd;
	struct sockaddr *address;
	int sockaddrlen;
}socketStruct;

typedef void *(*func_f)(void *);

void *listenConnection(socketStruct *mySock);
void addConnection(int socketNum);
llist *newConnection(int fileDesc);
void destroyLL(void);

void ignoreSIGINT(int sig_num)
{
	fprintf(stderr, "Must hit CTRL+D to exit.\n");
}

static llist *head = NULL;

int main(int argc, __attribute__((unused))  char **argv)
{
	if(argc > 1)
	{
		fprintf(stderr, "This program does not support arguments.\n");
		exit(1);
	}	
	struct sigaction ignore = {
		.sa_handler = &ignoreSIGINT,
		.sa_flags = SA_RESTART
	};
	sigaction(SIGINT, &ignore, NULL);
	char *end; 
	char *relayEnv = getenv("RELAY");
	if(!relayEnv)
	{
		fprintf(stderr, "RELAY environmental variable must be set.\n");
		exit(1);
	}
	long portNum = strtol(relayEnv, &end, 0);
	if(*end == '\0' && portNum > 10024)
	{
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		int opt = 1;
		if(fd == 0)
		{
			perror("Unable to create socket.\n");
			exit(1);
		}
		if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
				&opt, sizeof(opt)) != 0)
		{
			perror("Unable to set socket options.\n");
			exit(1);
		}
		struct sockaddr_in address = {
			.sin_family = AF_INET,
			.sin_addr.s_addr = INADDR_ANY,
			.sin_port = htons(portNum)
		};

		socketStruct mySock ={
			.socketFd = fd,
			.address = (struct sockaddr *)&address,
			.sockaddrlen = sizeof(address)
		};

		if(bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0)
		{
			perror("Unable to bind socket.\n");
			exit(1);
		}
		pthread_t p1;
		pthread_create(&p1, NULL, (func_f)listenConnection, &mySock);
		pthread_detach(p1);
		while(1)
		{
			char *line = NULL;
			long unsigned lineLen;
			int length = getline(&line, &lineLen, stdin);
			if(length == -1)
			{
				free(line);
				if(head != NULL)
				{
					destroyLL();
				}
				exit(1);
			}
			llist *index = head;
			while(index != NULL)
			{
				//check to see if still listening
				send(index->fileDesc, line, sizeof(line), 0);
				index = index->next;
			}
			free(line);
		}
	}
	else
	{
		fprintf(stderr, "RELAY variable must be a number above 10024.\n");
		exit(1);
	}
	return 0;
}

void *listenConnection(socketStruct *mySock)
{
	int socketNum = 0;
	listen(mySock->socketFd, 2);
	while(1)
	{
		socketNum = accept(mySock->socketFd, mySock->address, (socklen_t *)&mySock->sockaddrlen);
		if(socketNum > 0)
		{
			addConnection(socketNum);
		}
	}
}

void addConnection(int socketNum)
{
	llist *index = head;
	if(head == NULL)
	{
		head = newConnection(socketNum);
		return;
	}
	while(index->next != NULL)
	{
		index = index->next;
	}
	index->next = newConnection(socketNum);
}

llist *newConnection(int fileDesc)
{
	llist *new = malloc(sizeof(new));
	new->fileDesc = fileDesc;
	new->next = NULL;
	return new;
}

void destroyLL(void)
{
	llist *index = head;
	while(index->next != NULL)
	{
		index = head->next;
		free(head);
		head = index;
	}
	free(head);
}
