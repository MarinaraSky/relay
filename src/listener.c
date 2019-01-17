#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>

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

		int message = shmget(unique, 4096, 0666|IPC_CREAT);
		char *test = shmat(message, 0, 0);

		/*
		int testLock = shmget(unique, 1024, 0666|IPC_CREAT);
		sem_t lock = shmat(testLock, 0, 0);
		*/
		sem_t *lock = sem_open("/test1", O_RDWR);

		while(1)
		{
			sem_wait(lock);
			printf("Test: %s\n", test);
			sem_post(lock);
		}

		
		//shmdt(test);
	}
	else
	{
		fprintf(stderr, "RELAY environmental variable must be set.\n");
		exit(1);
	}
	return 0;
}

