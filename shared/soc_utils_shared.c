#include "shared_utils.c"

/*
    Gestisce il caso di chiusura non prevista
*/
void handler(int sig)
{
    printf("\nChiusura del dispositivo non permessa con queste modalita'\n");
    return;
}

/*
    Inizializzazione della struttura dati del socket associato al server.
*/
void init_server_addr(struct sockaddr_in *server_addr)
{
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_ADDRESS, &server_addr->sin_addr);
    return;
}

void init_stream_IO(struct Socket_stream_IO *elem)
{
    FD_ZERO(&elem->master);
    FD_ZERO(&elem->read_fds);
    FD_SET(elem->soc, &elem->master);
    FD_SET(STDIN_FILENO, &elem->master);
    elem->max_fd = elem->soc;
    return;
}