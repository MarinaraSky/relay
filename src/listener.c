#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


int main(int argc, __attribute__((unused)) char **argv)
{
	if(argc > 1)
	{
		fprintf(stderr, "This program does not support arguments.\n");
		exit(1);
	}	
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
		if(fd == 0)
		{
			perror("Unable to create socket.\n");
			exit(1);
		}
		struct sockaddr_in serverAddress = {
			.sin_family = AF_INET,
			.sin_port = htons(portNum)
		};
		if(inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) != 1)
		{
			fprintf(stderr, "Invalid server IP.\n");
			exit(1);
		}
		if(connect(fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
		{
			fprintf(stderr, "Cannot connect to server.\n");
			exit(1);
		}
		char received[4096] = {0};
		int numRecieved = 1;
		while(numRecieved > 0)
		{
			numRecieved = read(fd, received, 4095);
			printf("%s\n", received);
		}
	}
	else
	{
		fprintf(stderr, "RELAY environmental variable must be set.\n");
		exit(1);
	}
	return 0;
}

