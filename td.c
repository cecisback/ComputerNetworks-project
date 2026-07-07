#include "common.c"

void cleanup(struct select_elem *tb)
{
    close(tb->soc);
    FD_ZERO(&tb->read_fds);
    FD_ZERO(&tb->write_fds);
    remove("ord.txt");
    remove("recap.txt");
    remove("menu.txt");
    remove("ord_oftheday.txt");
    printf("Chiusura del dispositivo in corso...\n");
    fflush(stdout);
    return;
}

/*
    Prende il codice inserito da tastiera dall'utente e lo confronta
    con il codice della prenotazione prevista per quel tavolo e per
    quella data corrente.
*/
void enter_cod(struct select_elem *tb, char *buf, int id_b)
{
    int cod;
    char check;

    while (1)
    {
        memset(buf, 0, MAX_SIZE);
        cod = 0;

        if (fgets(buf, MAX_SIZE, stdin))
        {
            if (sscanf(buf, "%d%c", &cod, &check) == 2)
            {
                if ((cod && cod == id_b) && check == '\n')
                    return;
            }
        }
    }
    return;
}

/*
    Riceve dal server il codice di prenotazione previsto per quel tavolo
    e per la data e fascia oraria corrente
*/
void rcv_bk_oftheday(int sc, int *id_b)
{
    int ret;
    uint32_t conv;

    ret = recv(sc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione del codice di prenotazione\n");
        exit(1);
    }
    *id_b = ntohl(conv);
    printf("%d\n", *id_b); // cancella

    return;
}

/*
    Autenticazione del cliente per accedere ai servici del table device
*/
bool auth_client(char *buf, struct select_elem *tb)
{
    int id_b = 0;

    rcv_bk_oftheday(tb->soc, &id_b);
    if (!id_b)
    {
        printf("%s\n", "Non sono previste prenotazioni per questa data\n");
        fflush(stdout);
        return false;
    }

    printf("%s", "\nInserire il codice di prenotazione:\n\n");
    fflush(stdout);
    enter_cod(tb, buf, id_b);
    return true;
}

/*
    Stampa del menu
*/
void print_menu(char *buf)
{
    FILE *f = fopen("menu.txt", "r");
    if (!f)
    {
        perror("Errore nell'apertura del file menu.txt\n");
        exit(1);
    }

    rewind(f);
    while (!feof(f))
    {
        memset(buf, 0, MAX_SIZE);

        if (fscanf(f, " %[^\n] ", buf))
        {
            printf("%s\n", buf);
            fflush(stdout);
        }
    }
    fclose(f);
    printf("\n");
    fflush(stdout);
    return;
}

void rcv_menu(int sc, char *buf)
{
    int choice, ret;
    uint32_t conv;
    FILE *f;

    // invio richiesta menu
    choice = 2;
    conv = htonl(choice);
    ret = send(sc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio della richiesta menu\n");
        exit(1);
    }

    // ricevo menu
    ret = 0;
    rcv_file(sc, buf, &ret);

    f = fopen("menu.txt", "w");
    if (!f)
    {
        perror("Errore nell'apertura del file menu.txt\n");
        exit(1);
    }
    fprintf(f, "%s", buf);
    fclose(f);
    return;
}

void print_cmd()
{
    printf("%s", "1) help --> mostra i dettagli dei comandi\n");
    printf("%s", "2) menu --> mostra il menu dei piatti\n");
    printf("%s", "3) comanda --> invia una comanda\n");
    printf("%s", "4) conto --> chiede il conto\n\n");
    fflush(stdout);
    return;
}

/*
    Fase pre autenticazione del cliente con la richiesta al server
    del codice di prenotazione previsto per quel turno di lavoro
    (se ci sono eventualmente prenotazioni per quel tavolo)
*/
void beginning(struct select_elem *tb, char *buf)
{
    int choice, ret;
    uint32_t conv;

    // invio al server un segnale per ricevere il codice
    choice = -1;
    conv = htonl(choice);
    ret = send(tb->soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio della richiesta idb\n");
        exit(1);
    }

    if (auth_client(buf, tb))
    {
        rcv_menu(tb->soc, buf);
        printf("%s", "\n****************************** BENVENUTO ******************************\n");
        printf("%s", "Digita un comando:\n\n");
        print_cmd();
        printf("%s\n", buf); // stampa del menu'
        fflush(stdout);
    }
    return;
}

/*
    Controllo che il formato con il quale sono stati specificati i piatti
    nella comanda sia corretto.
    Il menu precedentemente ricevuto serve per controllare che non siano
    stati inseriti codici di piatti non presenti e dunque non ordinabili.
    Il vettore pre_count serve a tenere traccia, per ogni piatto specificato e
    presente nel menu',delle quantita' complessive richieste.
*/
bool check_format(int *pre_count, char **scorri)
{
    int i;
    char dish[MAX_SIZE];
    struct dishes ds_ord, ds_menu;

    FILE *f = fopen("menu.txt", "r");
    if (!f)
    {
        perror("Errore nell'apertura del file menu.txt\n");
        exit(1);
    }

    memset(&ds_ord, 0, sizeof(struct dishes));
    memset(&ds_menu, 0, sizeof(struct dishes));
    memset(dish, 0, MAX_SIZE);

    // preleva l'id del piatto ordinato con relativa quantita'
    while (sscanf(*scorri, " %[^\n ] ", dish) == 1)
    {
        if (sscanf(dish, " %[^-]-%d ", ds_ord.id, &ds_ord.quanti) == 2)
        {
            rewind(f);
            i = 0;
            // controllo l'id prelevato con ogni piatto presente nel menu'
            while (!feof(f))
            {
                if (fscanf(f, " %[^ ]%*[^\n] ", ds_menu.id))
                {
                    if (!strcmp(ds_menu.id, ds_ord.id))
                    {
                        pre_count[i] += ds_ord.quanti;
                        break;
                    }
                    i++;
                }
            }
            if (i == DIM_MENU)
                break;
        }
        else
        {
            i = DIM_MENU;
            break;
        }
        *scorri += (strlen(dish) + 1);
    }
    // in questo caso non e' stato trovato nel menu un piatto con quell'id
    // ed e' sufficiente uscire dal controllo senza procedere
    if (i == DIM_MENU)
    {
        fclose(f);
        printf("Comanda errata\n");
        fflush(stdout);
        return false;
    }

    fclose(f);
    return true;
}

void prp_ord(int soc, struct order *ord, char *buf, int port)
{
    int ret;
    uint32_t conv;

    // invio il numero di ordine effettuato
    ord->n_ord++;
    conv = htonl(ord->n_ord);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del numero di ordine\n");
        exit(1);
    }

    if (port == 5001)
    {
        ord->id_tb = 0;
    }
    else if (port == 5002)
    {
        ord->id_tb = 1;
    }
    else if (port == 5003)
    {
        ord->id_tb = 2;
    }

    conv = htonl(ord->id_tb);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio dell'id del tavolo\n");
        exit(1);
    }

    send_file(soc, buf, 0); // invio della comanda
    return;
}

void rcv_ACK_ord(int soc, char *dishes_list, struct order *ord)
{
    int ret;
    uint32_t conv;
    char ACK[ACK_FOR_ORD];

    memset(ACK, 0, ACK_FOR_ORD);

    ret = recv(soc, (void *)&ACK, ACK_FOR_ORD, 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione dell'ACK per la corretta ricezione dell'ordine\n");
        exit(1);
    }

    if (!strcmp(ACK, "CR\n"))
    {
        printf("COMANDA RICEVUTA\n");
        fflush(stdout);
        ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nella ricezione dell'id dell'ordine\n");
            exit(1);
        }
        ord->id_ord = ntohl(conv);
        save_ord(dishes_list, ord);
        printf("\n");
        fflush(stdout);
    }
    else
    {
        printf("Server non ha ricevuto correttamente la comanda\n");
        fflush(stdout);
    }
    return;
}

bool take_order(char *buf, struct order *ord)
{
    int i;
    char *scorri = &buf[8];
    int pre_count[DIM_MENU];

    memset(pre_count, 0, sizeof(pre_count));

    if (check_format(pre_count, &scorri))
    {
        for (i = 0; i < DIM_MENU; i++)
        {
            if (pre_count[i])
                ord->qty[i] += pre_count[i];
        }
        return true;
    }
    return false;
}

bool make_recap(int soc, char *buf, int *count)
{
    int i;
    FILE *f, *fs;
    char *scorri = &buf[0];
    bool ret = false;

    memset(buf, 0, MAX_SIZE);

    f = fopen("menu.txt", "r");
    if (!f)
    {
        perror("Errore nell'apertura del file menu.txt\n");
        exit(1);
    }
    rewind(f);

    fs = fopen("recap.txt", "a+");
    if (!f)
    {
        perror("Errore nell'apertura del file recap.txt\n");
        return false;
    }

    rewind(fs);
    for (i = 0; i < DIM_MENU; i++)
    {
        if (fscanf(f, " %2c%*[^\n] ", scorri))
        {
            if (count[i])
            {
                fprintf(fs, "%-3s %-4d\n", scorri, count[i]);
                ret = true;
            }
            else
                strcpy(scorri, "");
        }
    }
    fclose(fs);
    fclose(f);
    return ret;
}

void send_recap(int soc, char *buf)
{
    FILE *f;
    char *scorri = &buf[0];
    memset(buf, 0, MAX_SIZE);

    f = fopen("recap.txt", "r");
    if (f)
    {
        rewind(f);
        while (!feof(f))
        {
            if (fscanf(f, " %[^\n] ", scorri))
            {
                strcat(scorri, "\n");
                scorri += strlen(scorri);
            }
        }
        fclose(f);
    }
    send_file(soc, buf, 0);
    return;
}

void rcv_tot(int soc, char *buf)
{
    int ret;
    uint32_t conv;
    char *scorri = &buf[0];
    char dish[MAX_SIZE];

    memset(dish, 0, MAX_SIZE);

    while (sscanf(scorri, " %[^\n] ", dish) == 1)
    {
        printf("%s", dish);
        fflush(stdout);

        ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nella ricezione del totale per piatto\n");
            exit(1);
        }
        printf("%-d\n", ntohl(conv));
        fflush(stdout);
        scorri += (strlen(dish) + 1);
    }

    printf("%s ", "Totale: ");
    fflush(stdout);

    ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione del totale complessivo\n");
        exit(1);
    }

    printf("%-d\n\n", ntohl(conv));
    fflush(stdout);
    return;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in server_address, my_address;
    struct select_elem tb;
    struct order ord;
    int ret, choice, i, port;
    uint32_t conv;
    char buf[MAX_SIZE];

    memset(&tb, 0, sizeof(struct select_elem));
    memset(&ord, 0, sizeof(struct order));

    if (argc == 2)
    {
        port = atoi(argv[1]);
        if (!check_port(port, 1))
        {
            perror("Porta errata\n");
            exit(1);
        }
    }
    port = 5001;
    tb.soc = create_sc_client(&server_address, &my_address, port);
    beginning(&tb, buf);
    init_pre_select(&tb);

    for (;;)
    {
        tb.read_fds = tb.master;

        ret = select(tb.max_fd + 1, &tb.read_fds, NULL, NULL, NULL);
        if (ret < 0)
        {
            perror("Errore nella select\n");
            exit(1);
        }

        for (i = 0; i <= tb.max_fd; i++)
        {
            memset(buf, 0, MAX_SIZE);
            choice = 0;

            if (FD_ISSET(i, &tb.read_fds))
            {
                if (i == STDIN_FILENO)
                {
                    if (fgets(buf, MAX_SIZE, stdin))
                    {
                        if (!strcmp(buf, "help\n"))
                            print_cmd();
                        else if (!strcmp(buf, "menu\n"))
                            print_menu(buf);
                        else if (!strcmp(buf, "conto\n"))
                        {
                            if (check_sts_ord(buf))
                            {
                                if (make_recap(tb.soc, buf, ord.qty))
                                {
                                    choice = 4;
                                    conv = htonl(choice);
                                    ret = send(tb.soc, (void *)&conv, sizeof(uint32_t), 0);
                                    if (ret < 0)
                                    {
                                        perror("Errore nell'invio del comando\n");
                                        exit(1);
                                    }
                                    send_recap(tb.soc, buf);
                                    rcv_tot(tb.soc, buf);
                                    cleanup(&tb);
                                    return 0;
                                }
                                else
                                {
                                    printf("%s\n", "Non sono state effettuate ordinazioni");
                                    fflush(stdout);
                                }
                            }
                            else
                            {
                                printf("Non tutte le comande sono state servite\n");
                                fflush(stdout);
                            }
                        }
                        else if (!strncmp(buf, "comanda ", 8))
                        {
                            if (take_order(buf, &ord))
                            {
                                choice = 3;
                                conv = htonl(choice);
                                ret = send(tb.soc, (void *)&conv, sizeof(uint32_t), 0);
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del comando\n");
                                    exit(1);
                                }
                                prp_ord(tb.soc, &ord, &buf[8], port);
                                rcv_ACK_ord(tb.soc, &buf[8], &ord);
                            }
                        }
                    }
                }
                else
                {
                    ret = recv(tb.soc, (void *)&conv, sizeof(uint32_t), 0);
                    if (ret < 0)
                    {
                        perror("Errore nel messaggio ricevuto\n");
                        exit(1);
                    }

                    choice = ntohl(conv);
                    if (choice == -2)
                    {
                        cleanup(&tb);
                        return 0;
                    }
                    else if (choice == 1)
                    {
                        rcv_info_ord(&ord, tb.soc);
                        update_sts("ord_oftheday.txt", '1', &ord);
                    }
                    else if (choice == 2)
                    {
                        rcv_info_ord(&ord, tb.soc);
                        update_sts("ord_oftheday.txt", '2', &ord);
                    }
                }
            }
        }
    }
    return 0;
}
