#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <pthread.h>

/**
 * @file dispatcher.c
 * @author Jack Spence
 * @date 18 Jan 2019
 * @brief Message Server Software.
 * Will allow any number of hosts to connect and recieve messages
 */

/**
 * @struct llist
 * @brief Linked List to store socket file descriptors
 */
typedef struct llist
{
	int fileDesc;
	struct llist *next;
}llist;

/**
 * @struct socketStruct
 * @brief Socket structure used to get socket information to thread
 */
typedef struct socketStruct{
	int socketFd;
	struct sockaddr *address;
	int sockaddrlen;
}socketStruct;

/**
 * @brief typedef used to simplify function cast
 */
typedef void *(*func_f)(void *);

/**
 * @brief Used to run in thread.
 * Will listen on port specified by RESET environment variable and
 * add new connections to linked list.
 * @param mySock socketStruct that has socket details
 */
void *listenConnection(socketStruct *mySock);
/**
 * @brief Used to add file descriptor to linked list
 * @param socketNum Number returned from socket(2)
 */
void addConnection(int socketNum);
/**
 * @brief Creates a new linked list node with socket(2) file descriptor
 * @param fileDesc File Descriptor
 * @return New node for linked list
 */
llist *newConnection(int fileDesc);
/**
 * @brief Removes file descriptor from linked list
 */
void removeConnection(int fileDesc);
/**
 * @brief Frees memory from linked list
 */
void destroyLL(void);

/**
 * @brief Signal handler to catch CTRL+C
 */
void ignoreSIGINT(__attribute__((unused))int sig_num)
{
	fprintf(stderr, "Must hit CTRL+D to exit.\n");
}

/**
 * @brief Global static linked list for file descriptors.
 */
static llist *head = NULL;
/**
 * @brief Global static mutex lock to avoid linked list corruption
 */
static pthread_mutex_t lock;

/**
 * @brief Global static used to limit number of clients.
 * Went with 99 because it seemed like a good idea.
 */
static long maxConnection = 99;

/**
 * @brief Global static used to track number of clients.
 */
static long currentConnection = 0;

int main(int argc, __attribute__((unused))  char **argv)
{
	/**
	 * @brief Used for getopt and switch case.
	 */
	int opt = 0;
	/**
	 * @brief Used for string verification from command line
	 */
	char *pEnd;
	/**
	 * @brief Using getopt(3) to parse command line arguments
	 */
	while((opt = getopt(argc, argv, "hl:")) != -1)
	{
		switch(opt)
		{
			/**
			 * @brief Limit switch
			 */
			case 'l':
			{
				long newMax = strtol(optarg, &pEnd, 0);
				if(*pEnd == '\0' && newMax > 0)
				{
					maxConnection = newMax;		
				}
				break;
			}
			case 'h':
			{
				printf("USAGE: ./dispatcher [-l <limit>]\n");
				exit(1);
			}
			default:
			{
				fprintf(stderr, "USAGE: ./dispatcher [-l <limit>]\n");
				exit(1);
			}
		}
	}
	/**
	 * @brief Checking for other garbage not caught by getopt(3)
	 */
	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] != '-')
		{
			fprintf(stderr, "USAGE: ./dispatcher [-l <limit>]\n");
			exit(1);
		}
	}
	/**
	 * @brief Sigaction used to call signal hander function
	 */
	struct sigaction ignore = {
		.sa_handler = &ignoreSIGINT,
		.sa_flags = SA_RESTART
	};
	/**
	 * @brief Setting program to call sighandler for CTRL+C
	 */
	sigaction(SIGINT, &ignore, NULL);
	/**
	 * @brief Allows program to continue running after 
	 * SIGPIPE from closed connection.
	 */
	signal(SIGPIPE, SIG_IGN);
	/**
	 * @brief End pointer for getenv(3) verification.
	 */
	char *end; 
	/**
	 * @brief String pulled from getenv(3).
	 */
	char *relayEnv = getenv("RELAY");
	/* If RELAY was not found in environment variables. */
	if(!relayEnv)
	{
		fprintf(stderr, "RELAY environmental variable must be set.\n");
		exit(1);
	}
	/**
	 * @brief Port number as a long to connect to. 
	 * */
	long portNum = strtol(relayEnv, &end, 0);
	if(*end == '\0' && portNum > 10024 && portNum < 20025)
	{
		/**
		 * @brief File descriptor returned from socket(2).
		 */
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		int opt = 1;
		if(fd == 0)
		{
			perror("Unable to create socket.\n");
			exit(1);
		}
		/**
		 * @brief Setting the socket options.
		 */
		if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
				&opt, sizeof(opt)) != 0)
		{
			perror("Unable to set socket options.\n");
			exit(1);
		}
		/**
		 * @brief Initialization of socket structure
		 */
		struct sockaddr_in address = {
			.sin_family = AF_INET,
			.sin_addr.s_addr = INADDR_ANY,
			.sin_port = htons(portNum)
		};

		/**
		 * @brief Initialization of socketStruct for thread.
		 */
		socketStruct mySock ={
			.socketFd = fd,
			.address = (struct sockaddr *)&address,
			.sockaddrlen = sizeof(address)
		};

		/**
		 * @brief Binds to port specified in RELAY
		 */
		if(bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0)
		{
			perror("Unable to bind socket.\n");
			exit(1);
		}
		/**
		 * @brief Thread created to handle incoming connections
		 */
		pthread_t p1;
		/**
		 * @brief initialze mutex lock
		 */
		if(pthread_mutex_init(&lock, NULL) != 0)
		{
			perror("Mutex Failed.");
			exit(1);
		}
		/**
		 * @brief Spawns thread to run and handle incoming connection
		 */
		pthread_create(&p1, NULL, (func_f)listenConnection, &mySock);
		pthread_detach(p1);
		/**
		 * @brief Buffer to store input into.
		 */
		char *line = NULL;
		while(1)
		{
			/**
			 * @brief Set by getline(3).
			 */
			long unsigned lineLen;
			/**
			 * @brief Used to control how much data gets sent.
			 */
			int length = getline(&line, &lineLen, stdin);
			line[length] = '\0';
			/**
			 * @brief Exit condition.
			 * User entered CTRL+D
			 */
			if(length == -1)
			{
				free(line);
				if(head != NULL)
				{
					destroyLL();
				}
				pthread_mutex_destroy(&lock);
				exit(1);
			}
			/**
			 * @brief Not goint to talk to myself.
			 */
			if(currentConnection == 0)
			{
				continue;
			}
			/**
			 * @brief Iterator used for sending to each file descriptor
			 */
			llist *index = head;
			/**
			 * @brief Grabbing lock for critical code section
			 */
			pthread_mutex_lock(&lock);
			while(index != NULL)
			{
				/**
				 * @brief Sending data to clients
				 */
				int isSent = send(index->fileDesc, line, length, 0);
				/**
				 * @brief Checking if message was received.
				 * Removing connection from list if not received.
				 */
				if(isSent == -1)
				{
					removeConnection(index->fileDesc);
					currentConnection--;
				}
				index = index->next;
			}
			/**
			 * @brief Releasing lock.
			 */
			pthread_mutex_unlock(&lock);
			memset(line, 0, length);
		}
		free(line);
	}
	else
	{
		fprintf(stderr, "RELAY variable must be between 10025 and 20025.\n");
		exit(1);
	}
	return 0;
}

void *listenConnection(socketStruct *mySock)
{
	/**
	 * @brief Int used for file descriptor
	 */
	int socketNum = 0;
	/**
	 * @brief Starts listening on sockets file descriptor
	 */
	listen(mySock->socketFd, 2);
	const char *error = "Client max reached.";
	while(1)
	{
		/**
		 * @brief Accepts incoming connections from clients
		 */
		socketNum = accept(mySock->socketFd, mySock->address, (socklen_t *)&mySock->sockaddrlen);
		if(currentConnection >= maxConnection)
		{
			send(socketNum, error, strlen(error), 0);
			shutdown(socketNum, 2);
			return NULL;
		}
		if(socketNum > 0)
		{
			/**
			 * @brief Grabs lock and adds connection to linked list
			 */
			pthread_mutex_lock(&lock);
			addConnection(socketNum);
			currentConnection++;
			pthread_mutex_unlock(&lock);
		}
	}
}

void addConnection(int socketNum)
{
	/**
	 * @brief Iterator used for sending to each file descriptor
	 */
	llist *index = head;
	/**
	 * @brief Adding first connection to linked list
	 */
	if(head == NULL)
	{
		head = newConnection(socketNum);
		head->next = NULL;
		return;
	}
	/**
	 * @brief Finding end of linked list
	 */
	while(index->next != NULL)
	{
		index = index->next;
	}
	/**
	 * @brief Creating new node and adding to end of linked list
	 */
	index->next = newConnection(socketNum);
}

llist *newConnection(int fileDesc)
{
	/**
	 * @brief New node for linked list
	 */
	llist *new = malloc(sizeof(*new));
	/**
	 * @brief Obligatory checking return of malloc
	 */
	if(new == NULL)
	{
		perror("Out of Memory.");
		exit(1);
	}
	new->fileDesc = fileDesc;
	new->next = NULL;
	return new;
}

void removeConnection(int fileDesc)
{
	/**
	 * @brief Iterators used for sending to each file descriptor
	 */
	llist *index = head;
	llist *rem = index;
	/**
	 * @brief Checking if first in list is to be removed
	 */
	if(index->fileDesc == fileDesc)
	{
		rem = head;
		/**
		 * @brief Testing to see if only one in linked list.
		 */
		if(head->next == NULL)
		{
			head = NULL;
		}
		/**
		 * @brief If the first one needs to be removed.
		 */
		else
		{
			head = head->next;
		}
		free(rem);
		return;
	}
	/**
	 * @brief Looping to find fileDesc to remove from list
	 */
	while(index->next->fileDesc != fileDesc)
	{
		index = index->next;
	}
	rem = index;
	index->next = index->next->next;
	free(rem);
}

void destroyLL(void)
{
	/**
	 * @brief Iterator used for sending to each file descriptor
	 */
	llist *index = head;
	/**
	 * @brief Loop used to free each element in linked list
	 */
	while(index->next != NULL)
	{
		index = head->next;
		free(head);
		head = index;
	}
	free(head);
}
