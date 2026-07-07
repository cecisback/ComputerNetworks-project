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
#define DATE_SIZE 4
#define NUM_TABLE 4
#define MAX_DIM_SURNAME 20
#define ACK_FOR_BOOK 23
#define ACK_FOR_ORD 4 // CR\n
#define INFO_SIZE 5
#define DIM_ROW_SUMM 9
#define DIM_MENU 8
#define N_PORT 7
#define MAX_DIM_ID_DISH 3

const int idt_tot[NUM_TABLE] = {1, 4, 23, 8};

struct tm *current_date()
{
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    return timeinfo;
}

struct dishes
{
    char id[MAX_DIM_ID_DISH];
    int quanti;
};

struct receipt
{
    int tot_ord;
    int tot_ds[DIM_MENU];
    int num_ds_rcp;
};

struct order
{
    int qty[DIM_MENU];
    int id_ord;
    char sts;
    int id_tb;
    int n_ord;
};

struct detail_bk
{
    int id_b;
    int opz_choose;
    int numppl;
    char surname[MAX_DIM_SURNAME];
};

struct booking
{
    // 0:giorno, 1:mese, 2:anno, 3:fascia oraria
    int date[DATE_SIZE];
    int t_opz[NUM_TABLE];
    int n_opz;
    struct detail_bk dbk;
};

struct workshift
{
    int date[DATE_SIZE];
    int id_bk[NUM_TABLE];
    int tot_pnd_ord;
};

struct select_elem
{
    fd_set master;
    fd_set read_fds;
    fd_set write_fds;
    int soc;
    int max_fd;
};

struct server_r_info
{
    struct sockaddr_in myaddr;

    // soc corrispone nel server al socket di ascolto
    struct select_elem ser;

    // per salvare informazioni sulla connessione dei client
    // che si connettono su specifiche porte a seconda del servizio
    // richiesto
    int soc_dev[N_PORT];
};
