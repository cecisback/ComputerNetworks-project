#include "../communication_utilities/soc_client_utils.c"

#define CLIENT_PORT 7000

#define SURNAME_SIZE 20
#define DATE_SIZE 4

/*
    Command IDs exchanged between client-server processes generated 
    from the execution of cli.c and server.c
*/

#define FIND_CMD 1
#define BOOK_CMD 2
#define ESC_CMD 3
#define RESERVATION_ACK 4

struct Reservation
{
    int id;
    int selected_table;
    int num_guests;
    char surname[SURNAME_SIZE];
};

struct Reservation_request
{
    int date[DATE_SIZE];
    int available_table_IDs[NUM_TABLE];
    int tot_available_tables;
    struct Reservation reservation_info;
};