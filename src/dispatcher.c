#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

void ignoreSIGINT(int sig_num)
{
	fprintf(stderr, "Must hit CTRL+D to exit.\n");
}

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
	char *keyValue = getenv("RELAY");
	if(keyValue)
	{
		key_t m_unique = ftok(keyValue, 99);

		int message = shmget(m_unique, 4096, 0666|IPC_CREAT);
		char *test = shmat(message, 0, 0);

		/*
		key_t l_unique = ftok(keyValue, 66);
		int testLock = shmget(l_unique, 1024, 0666|IPC_CREAT);
		sem_t lock = shmat(testLock, 0, 0);
		sem_init(&lock, 1, 0);
		*/

		sem_t *lock = sem_open("/test1", O_RDWR | O_CREAT | O_EXCL, 0666, 1);

		
		int readRet = 0;
		long unsigned len;
		while(readRet != -1)
		{
			sem_wait(lock);
			printf("Ready For Message.\n");
			readRet = getline(&test, &len, stdin);
			sem_post(lock);
		}

		
		//shmdt(test);
		//shmdt(lock);
		sem_unlink("/test1");
	}
	else
	{
		fprintf(stderr, "RELAY environmental variable must be set.\n");
		exit(1);
	}
	return 0;
}

