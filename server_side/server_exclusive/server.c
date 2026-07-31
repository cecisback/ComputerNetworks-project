#include "server_utilities.c"

int main(int argc, char *argv[])
{
    int i, ret, command, port, outcome_auth;
    uint32_t conv;

    struct Reservation_request req;
    struct Order ord;
    struct Workshift ws;
    struct Receipt rcp;
    struct Server_r_info r;

    char buf[MAX_SIZE];

    memset(&ord, 0, sizeof(struct Order));
    memset(buf, 0, MAX_SIZE);

    signal(SIGINT, handler);

    if (argc == 2)
    {
        port = atoi(argv[1]);
        if (!check_port_server(port))
            ;
        {
            perror("Porta errata\n");
            exit(1);
        }
    }

    start_server(&r, &ws);

    for (;;)
    {
        r.server_IO.read_fds = r.server_IO.master;

        ret = select(r.server_IO.max_fd + 1, &r.server_IO.read_fds, NULL, NULL, NULL);
        if (ret < 0)
        {
            perror("Server non attivo\n");
            exit(1);
        }

        for (i = 0; i <= r.server_IO.max_fd; i++)
        {
            command = 0;

            memset(buf, 0, MAX_SIZE);

            if (FD_ISSET(i, &r.server_IO.read_fds))
            {
                if (i == STDIN_FILENO)
                {
                    if (read(STDIN_FILENO, buf, MAX_SIZE) > 0)
                    {
                        if (!strcmp(buf, "stop\n"))
                        {
                            stop_server(&r);
                            return 0;
                        }
                        else
                        {
                            check_input(buf);
                        }
                    }
                }
                // richiesta di connessione al socket di ascolto
                else if (i == r.server_IO.soc)
                    accept_new_connection(&r);
                else if (i == r.sockets_server[0])
                {
                    ret = recv(r.sockets_server[0], (void *)&conv, sizeof(uint32_t), 0);
                    if (ret < 0)
                    {
                        perror("Socket chiuso");
                        r.sockets_server[0] = 0;
                        exit(1);
                    }

                    command = ntohl(conv);
                    switch (command)
                    {
                    case FIND_CMD:
                        printf("%s%d\n", "Elaborazione richiesta prenotazione per socket ", r.sockets_server[0]);
                        fflush(stdout);
                        send_reservation_proposal(&req, r.sockets_server[0], buf);
                        break;
                    case BOOK_CMD:
                        save_new_reservation(r.sockets_server[0], &req, &ws);
                        printf("%s%d\n", "Aggiunta nuova prenotazione, id: ", req.reservation_info.id);
                        fflush(stdout);
                        send_ACK_new_reservation(buf, &req, r.sockets_server[0]);
                        break;
                    case ESC_CMD:
                        close(i);
                        printf("%s%d\n", "Chiusura del socket client ", r.sockets_server[0]);
                        fflush(stdout);
                        FD_CLR(i, &r.server_IO.master);
                        r.sockets_server[0] = 0;
                        break;
                    }
                }
                else if (i == r.sockets_server[1] || i == r.sockets_server[2] || i == r.sockets_server[3])
                {
                    int id_table = -1;
                    int id_soc = 1;

                    for (int j = id_soc; j < N_PORT-3; j++)
                    {
                        if (i == r.sockets_server[j]){
                            id_soc = j--;
                            id_table = tables_ID_list[j];
                            break;
                        }
                    }

                    if (id_table == -1){
                        printf("%s%d%s\n","Tavolo corrispondente al socket ",i," non trovato");
                        exit(1);
                    }

                    ret = recv(i, (void *)&conv, sizeof(uint32_t), 0);
                    if (ret < 0)
                    {
                        perror("Socket chiuso\n");
                        exit(1);
                    }

                    command = ntohl(conv);
                    switch (command){
                    case TB_AUTHENTICATION_CMD:
                        conv = htonl(ws.reservation_ID[id_table]);
                        ret = send(i, (void *)&conv, sizeof(uint32_t), 0);
                        if (ret < 0)
                        {
                            perror("Errore nell'invio dell'ID di prenotazione\n");
                            exit(1);
                        }
                        break;
                    case MENU_CMD:
                        printf("%s%d\n", "Invio menu al socket ", i);
                        fflush(stdout);
                        send_menu(i, buf);
                        break;
                    case COMANDA_CMD:
                        printf("%s%d\n", "Ricevuta una comanda dal socket ", i);
                        fflush(stdout);

                        receive_order(i, buf, &ord);
                        ord.table_ID = id_table;
                        store_waiting_order(buf, &ord);
                        send_ACK_order(i, ord.order_ID);
                        ws.tot_open_orders++;

                        printf("%s%d\n\n", "Totale ordini pendenti: ", ws.tot_open_orders);
                        fflush(stdout);
                        notify_n_remaining_pending_orders(r.sockets_server, ws.tot_open_orders);
                        break;
                    case END_TB_CONNECTION:
                        memset(&rcp, 0, sizeof(struct Receipt));
                        if (receive_recap_order(i, buf))
                        {
                            make_receipt(buf, &rcp);
                            send_receipt(i, &rcp);
                            save_receipt(0, buf, &rcp);
                        }
                        else
                        {
                            printf("%s%d\n", "Non sono presenti ordinazioni per il socket ", i);
                            fflush(stdout);
                        }
                        close(i);
                        printf("%s%d\n", "Chiusura del socket ", i);
                        fflush(stdout);
                        FD_CLR(i, &r.server_IO.master);
                        id_soc++;
                        r.sockets_server[id_soc] = 0;
                        break;
                    default:
                        break;
                    }
                }
                else if (i == r.sockets_server[4] || i == r.sockets_server[5])
                {
                    ret = recv(i, (void *)&conv, sizeof(uint32_t), 0);
                    if (ret < 0)
                    {
                        perror("Errore nella ricezione dei dati\n");
                        exit(1);
                    }

                    command = ntohl(conv);
                    switch (command)
                    {
                    case START_KC_DEVICE:
                        conv = htonl(ws.tot_open_orders);
                        ret = send(i, (void *)&conv, sizeof(uint32_t), 0);
                        if (ret < 0)
                        {
                            perror("Errore nell'invio del numero di ordini pendenti\n");
                            exit(1);
                        }
                        break;
                    case UPDATE_STATUS_ORD_1:
                        if (ws.tot_open_orders)
                        {
                            find_oldest_pending_order(buf, &ord);
                            printf("%s\n", buf);
                            fflush(stdout);

                            if (ord.status == PREPARING)
                            {
                                ws.tot_open_orders--;

                                // invio al tb device l'aggiornamento dello stato
                                send_info_order_to_tb(r.sockets_server, UPDATE_STATUS_ORD_1, &ord);
                                send_order_to_ktc(i, buf);
                                notify_n_remaining_pending_orders(r.sockets_server, ws.tot_open_orders);
                                printf("%s%d\n", "Ordini pendenti restanti ", ws.tot_open_orders);
                                fflush(stdout);
                            }
                        }
                        else
                        {
                            printf("Non ci sono ordini pendenti\n");
                            fflush(stdout);
                        }
                        break;
                    case UPDATE_STATUS_ORD_2:
                        memset(&ord, 0, sizeof(struct Order));
                        receive_info_order(&ord, i);
                        update_status_order("ord_oftheday.txt", '2', &ord);
                        
                        if (!ord.order_ID)
                        {
                            printf("Impossibile aggiornare la comanda\n");
                            fflush(stdout);
                        }
                        else
                        {
                            send_info_order_to_tb(r.sockets_server, UPDATE_STATUS_ORD_2, &ord);
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
        }
    }
    return 0;
}
