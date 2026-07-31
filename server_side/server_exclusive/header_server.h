#include "../../client_side/headers/header_client.h"
#include "../../client_side/headers/header_kitchen.h"
#include "../../client_side/headers/header_order.h"
#include "../../client_side/headers/header_table_device.h"
#include "../../client_side/order_shared_utils.c"
#include "../../server_side/order_shared_utils.c"

#define IDX_FIRST_KTC 4
#define IDX_LAST_KTC 6

#define INFO_SIZE 5
#define N_PORT 7

const int tables_ID_list[NUM_TABLE] = {1, 4, 23, 8};

struct Server_r_info
{
    struct sockaddr_in myaddr;

    /* Strutture dati per l'esecuzione di operazioni I/O con le nuove connessioni client accettate
     sulle porte di ascolto*/
    struct Socket_stream_IO server_IO;

    /* Per salvare (server side) informazioni sulla connessione dei client
       che si connettono su specifiche porte a seconda del servizio richiesto*/
    int sockets_server[N_PORT];
};

struct Workshift
{
    int date[DATE_SIZE];
    int reservation_ID[NUM_TABLE];
    int tot_open_orders;
};