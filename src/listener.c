#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

int main(int argc, __attribute__((unused)) char **argv)
{
	if(argc > 1)
	{
		fprintf(stderr, "This program does not support arguments.\n");
		exit(1);
	}	
	char *keyValue = getenv("RELAY");
	if(keyValue)
	{
		key_t unique = ftok(keyValue, 99);
		int mmId = shmget(unique, 4096, 0666|IPC_CREAT);
		char *test = shmat(mmId, 0, 0);
		printf("Test: %s\n", test);

		
		shmdt(test);
	}
	else
	{
		fprintf(stderr, "RELAY environmental variable must be set.\n");
		exit(1);
	}
	return 0;
}

