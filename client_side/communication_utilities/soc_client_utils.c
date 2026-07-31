#include "../../shared/soc_utils_shared.c"

/*
    Inizializza l'indirizzo del client con la porta opportuna
*/
void init_client_addr(struct sockaddr_in *my_address, int port)
{
    memset(my_address, 0, sizeof(*my_address));
    my_address->sin_family = AF_INET;
    my_address->sin_port = htons(port);
    return;
}

/*
    Connette l'indirizzo creato al socket
*/
int bind_to_soc(struct sockaddr_in *my_address)
{
    int soc, ret;

    soc = socket(AF_INET, SOCK_STREAM, 0);
    if (soc < 0)
    {
        perror("Errore nella creazione del socket\n");
        exit(1);
    }

    ret = bind(soc, (struct sockaddr *)my_address, sizeof(*my_address));
    if (ret < 0)
    {
        fprintf(stderr, "%s%d\n", "Errore nella bind alla porta ", errno);
        exit(1);
    }
    return soc;
}

/*
    Crea il socket client side. 
    Inizializza le informazioni relative all'indirizzo del server e quelle relative all'indirizzo del client.
    Fa binding e connette i due socket comunicanti con protocollo TCP.
*/
int create_soc_client(struct sockaddr_in *s_addr, struct sockaddr_in *c_addr, int port)
{
    int sc, ret;
    init_server_addr(s_addr);
    init_client_addr(c_addr, port);
    sc = bind_to_soc(c_addr);

    ret = connect(sc, (struct sockaddr *)s_addr, sizeof(*s_addr));
    if (ret < 0)
    {
        fprintf(stderr, "%s%s\n", "Errore di connessione alla porta ", strerror(errno));
        exit(1);
    }
    return sc;
}