#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_SIZE 1024
#define MENU_SIZE 8
#define NUM_TABLE 4

/* Da tenere con server_address*/
#define SERVER_PORT 4242
#define SERVER_ADDRESS "10.0.2.100"

#define STOP_SERVER_CMD -7
#define AUTHENTICATION_FAILED_CODE 1
#define AUTHENTICATION_ACK 2

/*
    Per ricavare informazioni relative alla data odierna.
*/
struct tm *current_date()
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    return timeinfo;
}

struct Socket_stream_IO
{
    fd_set master;
    fd_set read_fds;
    fd_set write_fds;
    int soc;
    int max_fd;
};