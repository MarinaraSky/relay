#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>


int
main(
    int argc,
    __attribute__ ((unused))
    char **argv)
{
    if (argc > 1)
    {
        fprintf(stderr, "This program does not support arguments.\n");
        exit(1);
    }
    /**
	 * @brief End pointer for getenv(3) verification.
	 */
    char           *end;

    /**
	 * @brief String pulled from getenv(3).
	 */
    char           *relayEnv = getenv("RELAY");

    /* If RELAY was not found in environment variables. */
    if (!relayEnv)
    {
        fprintf(stderr, "RELAY environmental variable must be set.\n");
        exit(1);
    }
    /**
	 * @brief Port number as a long to connect to. 
	 * */
    long            portNum = strtol(relayEnv, &end, 0);

    if (*end == '\0' && portNum > 10024 && portNum < 20025)
    {
        /**
		 * @brief File descriptor returned from socket(2).
		 */
        int             fd = socket(AF_INET, SOCK_STREAM, 0);

        if (fd == 0)
        {
            perror("Unable to create socket.\n");
            exit(1);
        }
        /**
		 * @brief Structure used to connect to server
		 */
        struct sockaddr_in serverAddress = {
            .sin_family = AF_INET,
            .sin_port = htons(portNum)
        };
        /**
		 * @brief Used to get localhost IP into bits
		 */
        if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) != 1)
        {
            fprintf(stderr, "Invalid server IP.\n");
            exit(1);
        }
        /**
		 * @brief Attempting to connect to server with previous struct
		 */
        if (connect
            (fd, (struct sockaddr *) &serverAddress,
             sizeof(serverAddress)) < 0)
        {
            fprintf(stderr, "Cannot connect to server.\n");
            exit(1);
        }
        /**
		 * @brief Buffer to store data from server
		 */
        char            recieved[4096] = { 0 };
        /**
		 * @brief Integer used to break loop if server closes.
		 */
        int             numRecieved = 1;

        while (numRecieved > 0)
        {
            memset(recieved, 0, sizeof(recieved));
            numRecieved = read(fd, recieved, 4095);
            printf("%s", recieved);
        }
    }
    else
    {
        /**
		 * @brief Error if RELAY isn't found
		 */
        fprintf(stderr,
                "RELAY environmental variable must be"
                "between 10025 and 20025.\n");
        exit(1);
    }
    return 0;
}
