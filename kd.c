#include "client_side/communication_utilities/soc_client_utils.c"
#include "client_side/headers/header_order.h"
#include "client_side/headers/header_kitchen.h"
#include "client_side/order_shared_utils.c"
#include "server_side/order_shared_utils.c"

void print_cmd()
{
    printf("%s", "take -. accetta una comanda\n");
    printf("%s", "show -. mostra le comande accettate (in preparazione)\n");
    printf("%s", "set -. imposta lo stato della comanda\n");
    printf("%s", "\n");
    fflush(stdout);
    return;
}

void receive_open_orders(int soc)
{
    int ret, i, pnd;
    uint32_t conv;

    ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        printf("Server non attivo\n");
        fflush(stdout);
        exit(1);
    }
    pnd = ntohl(conv);

    for (i = 0; i < pnd; i++)
    {
        printf("*");
        fflush(stdout);
    }

    printf("\n");
    fflush(stdout);
    return;
}

void show_accepted_order(char *buf, struct Order *ord)
{
    FILE *f = fopen("accepted.txt", "r");
    if (!f)
    {
        printf("Non ci sono comande accettate\n");
        fflush(stdout);
        return;
    }

    bool check = false;

    memset(buf, 0, MAX_SIZE);

    rewind(f);
    while (!feof(f))
    {
        if (fscanf(f, " %[^\n] ", buf))
        {
            if (sscanf(buf, "ID:%*3c com%*d T%*d STATUS = %d\n", &ord->status))
            {
                if (ord->status == 1)
                {
                    strcpy(&buf[16], "");
                    check = true;
                }
                else
                    check = false;
            }

            if (check == true)
            {
                printf("%s\n", buf);
                fflush(stdout);
            }
        }
    }
    fclose(f);
    return;
}

void store_accepted_order(char *buf)
{
    char ord[MAX_SIZE];
    memset(ord, 0, MAX_SIZE);

    FILE *f = fopen("accepted.txt", "a+");
    if (!f)
    {
        perror("Impossibile creare il file accepted.txt");
        exit(1);
    }

    sscanf(buf, " %[^\n] ", ord);
    fprintf(f, "%s STATUS = %d\n", ord, 1);
    fprintf(f, "%s", &buf[16]);
    fclose(f);
    return;
}

/*
    Controlla che la porta associata dall'utente al momento
    della chiamata al programma sia effettivamente corretta
*/
bool check_port_kitchen(int port)
{
    if (port == KTC_1_PORT || port == KTC_2_PORT)
        return true;

    return false;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in server_address, my_address;
    struct Order ord;
    int ret, command, i, port;

    char buf[MAX_SIZE];
    struct Socket_stream_IO kd;

    uint32_t conv;

    memset(&kd, 0, sizeof(struct Socket_stream_IO));

    if (argc == 2)
    {
        port = atoi(argv[1]);
        if (!check_port_kitchen(port))
        {
            perror("Porta errata\n");
            exit(1);
        }
    }

    kd.soc = create_soc_client(&server_address, &my_address, 6001);
    init_stream_IO(&kd);
    print_cmd();

    // invio segnale per ricevere gli ordini pendenti
    conv = htonl(START_KC_DEVICE);
    ret = send(kd.soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        printf("Errore nell'invio del segnale per la ricezione degli ordini pendenti\n");
        exit(1);
    }
    receive_open_orders(kd.soc);

    for (;;)
    {
        kd.read_fds = kd.master;

        ret = select(kd.max_fd + 1, &kd.read_fds, NULL, NULL, NULL);
        if (ret < 0)
        {
            perror("Errore nella select\n");
            exit(1);
        }

        for (i = 0; i <= kd.max_fd; i++)
        {
            memset(buf, 0, MAX_SIZE);
            memset(&ord, 0, sizeof(struct Order));

            command = 0;

            if (FD_ISSET(i, &kd.read_fds))
            {
                if (i == kd.soc)
                {
                    ret = recv(kd.soc, (void *)&conv, sizeof(uint32_t), 0);
                    if (ret < 0)
                    {
                        perror("Errore nella ricezione del comando\n");
                        exit(1);
                    }

                    command = ntohl(conv);
                    if (command == CLOSE_KD_SOCKET_CMD || command == STOP_SERVER_CMD)
                    {
                        printf("Chiusura del dispositivo in corso...\n");
                        fflush(stdout);
                        remove("accepted.txt");
                        close(kd.soc);

                        FD_ZERO(&kd.master);
                        FD_ZERO(&kd.read_fds);
                        return 0;
                    }
                    else if (command == SEND_OPEN_ORDERS_CMD)
                    {
                        receive_open_orders(kd.soc);
                    }
                    else if (command == UPDATE_STATUS_ORD_1)
                    {
                        ret = 0;
                        receive_file(kd.soc, buf, &ret);
                        printf("%s\n", buf);
                        fflush(stdout);
                        store_accepted_order(buf);
                    }
                }
                else if (i == STDIN_FILENO)
                {
                    if (fgets(buf, MAX_SIZE, stdin))
                    {
                        if (!strcmp(buf, "take\n"))
                        {
                            command = TAKE_CMD;
                            conv = htonl(command);
                            ret = send(kd.soc, (void *)&conv, sizeof(uint32_t), 0);
                            if (ret < 0)
                            {
                                perror("Errore nell'invio del comando digitato\n");
                                exit(1);
                            }
                        }
                        else if (!strcmp(buf, "show\n"))
                        {
                            show_accepted_order(buf, &ord);
                        }
                        else if (sscanf(buf, "set com%d-T%d\n", &ord.order_counter, &ord.table_ID) == 2)
                        {
                            ord.status = 1;
                            update_status_order("accepted.txt", '2', &ord);
                            if (ord.order_ID)
                            {
                                conv = htonl(UPDATE_STATUS_ORD_2);
                                ret = send(kd.soc, (void *)&conv, sizeof(uint32_t), 0);
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del comando digitato\n");
                                    exit(1);
                                }
                                send_info_order(&ord, kd.soc);
                            }
                            else
                            {
                                printf("%s\n", "Non e' possibile aggiornare lo stato dell'ordine: in preparazione -> in servizio.\n");
                                fflush(stdout);
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
