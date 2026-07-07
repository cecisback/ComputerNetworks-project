#include "common.c"

/*
    Creazione del file con le informazioni relative ai tavoli.
*/
void init_info_tb()
{
    FILE *f = fopen("info_tb.txt", "w");
    if (!f)
    {
        perror("Errore nella scrittura del file info_tb.txt\n");
        exit(1);
    }

    fprintf(f, "%-30s\n", "T1 SALA1 FINESTRA");
    fprintf(f, "%-30s\n", "T4 SALA1 CAMINO");
    fprintf(f, "%-30s\n", "T23 SALA2 PORTA_INGRESSO");
    fprintf(f, "%-30s\n", "T8 SALA2 PORTA_INGRESSO");
    fflush(stdout);
    fclose(f);
    return;
}

/*
    Creazione del file contenente il menu
*/
void init_menu()
{
    FILE *f;
    f = fopen("menu.txt", "w");
    if (!f)
    {
        perror("Errore nella scrittura del file menu\n");
        exit(1);
    }

    fprintf(f, "%-3s - %-30s%3d\n", "A1", "Antipasto di terra", 7);
    fprintf(f, "%-3s - %-30s%3d\n", "A2", "Antipasto di mare", 8);
    fprintf(f, "%-3s - %-30s%3d\n", "P1", "Spaghetti alle vongole", 10);
    fprintf(f, "%-3s - %-30s%3d\n", "P2", "Rigatoni all'amatriciana", 6);
    fprintf(f, "%-3s - %-30s%3d\n", "S1", "Frittura di calamari", 20);
    fprintf(f, "%-3s - %-30s%3d\n", "S2", "Arrosto misto", 5);
    fprintf(f, "%-3s - %-30s%3d\n", "D1", "Crostata di mele", 5);
    fprintf(f, "%-3s - %-30s%3d\n", "D2", "Zuppa inglese", 5);

    fclose(f);
    return;
}

/*
    Inizializzo la struttura workshift con le informazioni relative
    alla data odierna, al turno di lavoro, prenotazioni previste per
    ciascun tavolo.
*/
void init_info_workshift(struct workshift *ws)
{
    struct tm *timeinfo = current_date();
    ws->date[0] = timeinfo->tm_mday;
    ws->date[1] = timeinfo->tm_mon + 1;
    ws->date[2] = timeinfo->tm_year - 100;
    ws->date[3] = 0; // MODIFICA
    find_booked(ws->date, ws->id_bk, 0);
    return;
}

void print_cmd()
{
    printf("%s", "\nCOMANDI\n");
    printf("%s", "stat - restituisce lo stato di tutte le comande giornaliere\n");
    printf("%s", "stat table - mostra le comande relative al tavolo table\n");
    printf("%s", "status a|p|s - mostra le comande in attesa|in preparazione|in servizio\n");
    printf("%s", "stop - il server si arresta se non ci sono comande in preparazione o attesa\n");
    printf("%s", "\n");
    fflush(stdout);
    return;
}

/*
    Avvio del server
*/
void start(struct server_r_info *r, struct workshift *ws)
{
    memset(r, 0, sizeof(struct server_r_info));
    memset(ws, 0, sizeof(struct workshift));

    printf("%s\n", "Avvio del server...");
    fflush(stdout);

    init_info_tb();
    printf("%s\n", "Creazione del file contenente i dettagli dei tavoli...");
    fflush(stdout);

    init_info_workshift(ws);
    printf("%s\n", "Recupero informazioni sul turno di lavoro corrente...");
    fflush(stdout);

    init_menu();
    printf("%s\n", "Creazione del file contenente il menu...");
    fflush(stdout);

    init_server_addr(&r->myaddr);
    r->ser.soc = bind_to_soc(&r->myaddr);
    listen(r->ser.soc, 10);
    printf("%s\n", "Creazione del socket di ascolto..");
    fflush(stdout);
    init_pre_select(&r->ser);
    print_cmd();
    return;
}

void save_info_conn(int soc, int port, int *soc_dev)
{
    int i;

    switch (port)
    {
    case 7000:
        i = 0;
        break;
    case 5001:
        i = 1;
        break;
    case 5002:
        i = 2;
        break;
    case 5003:
        i = 3;
        break;
    case 6001:
        i = 4;
        break;
    case 6002:
        i = 5;
        break;
    };

    soc_dev[i] = soc;

    printf("%s\n", "Salvataggio delle informazioni sulla nuova connessione...");
    fflush(stdout);
    return;
}

void accept_new_conn(struct server_r_info *r)
{
    struct sockaddr_in cl_address;
    int len, soc, port;

    len = sizeof(cl_address);
    memset(&cl_address, 0, sizeof(struct sockaddr_in));

    soc = accept(r->ser.soc, (struct sockaddr *)&cl_address, (socklen_t *)&len);
    port = ntohs(cl_address.sin_port);
    printf("%s %d\n", "Nuova connessione su porta:", port);
    fflush(stdout);

    save_info_conn(soc, port, r->soc_dev);
    FD_SET(soc, &r->ser.master);

    if (soc > r->ser.max_fd)
        r->ser.max_fd = soc;
    return;
}
/*
    Mostra tutti gli elementi salvati nel file n_file
*/
void show_all(char *buf)
{
    char sts;
    char *scorri;

    FILE *f = fopen("ord_oftheday.txt", "r");
    if (!f)
    {
        printf("Non ci sono prenotazioni\n");
        return;
    }

    rewind(f);
    while (!feof(f))
    {
        memset(buf, 0, MAX_SIZE);
        scorri = &buf[0];

        if (fscanf(f, " %[^\n] ", scorri))
        {
            if (!strncmp(scorri, "ID:", 3))
            {
                if (sscanf(scorri, "%*6c com%*2d T%*2d STATUS = %c", &sts))
                {
                    scorri += 16;
                    strcpy(scorri, "");
                    printf("%s ", buf);
                    fflush(stdout);
                    show_sts_ord(sts);
                }
            }
            else
            {
                printf("%s\n", scorri);
                fflush(stdout);
            }
        }
    }
    fclose(f);
    return;
}

/*
    Controllo input comandi del server
*/
void check_input(char *buf)
{
    int id_tb, i;

    if (!strcmp(buf, "status p\n"))
        show_ord(0, '1', buf);
    else if (!strcmp(buf, "status a\n"))
        show_ord(0, '0', buf);
    else if (!strcmp(buf, "status s\n"))
        show_ord(0, '2', buf);
    else if (!strcmp(buf, "stat\n"))
    {
        show_all(buf);
    }
    else if (sscanf(buf, "stat T%d\n", &id_tb))
    {
        for (i = 0; i < NUM_TABLE; i++)
        {
            if (id_tb == idt_tot[i])
                break;
        }

        if (i < NUM_TABLE)
            show_ord(id_tb, ' ', buf);
        else
        {
            printf("Tavolo inserito non presente\n");
            fflush(stdout);
        }
    }
    return;
}

/*
    Spegne il server e lo notifica a tutti i restanti dispositivi
    connessi.
*/
void stop(struct server_r_info *r)
{
    int i, ret, choice;
    uint32_t conv;

    choice = -2;
    remove("ord_oftheday.txt");
    remove("menu.txt");
    remove("info_tb.txt");

    printf("Eliminazione del file ord_oftheday.txt...\n");
    printf("Eliminazione del file menu.txt...\n");
    printf("Eliminazione del file info_tb.txt...\n");
    fflush(stdout);

    // if (check_sts_ord("ord_oftheday.txt"))
    //{
    for (i = 0; i < N_PORT; i++)
    {
        if (r->soc_dev[i])
        {
            conv = htonl(choice);
            ret = send(r->soc_dev[i], (void *)&conv, sizeof(uint32_t), 0);
            if (ret < 0)
            {
                perror("Errore nell'invio della richiesta di spegnimento dispositivo\n");
                exit(1);
            }

            close(r->soc_dev[i]);
            FD_CLR(r->soc_dev[i], &r->ser.master);
            printf("%s%d%s\n", "Socket n. ", r->soc_dev[i], " chiuso\n");
            fflush(stdout);
        }
    }

    FD_ZERO(&r->ser.read_fds);
    FD_ZERO(&r->ser.write_fds);
    r->ser.max_fd = 0;
    r->ser.soc = 0;
    //}
    return;
}

/*
    Riceve le informazioni inserite dall'utente per la prenotazione,
    comprendenti la data e il numero di persone.
*/
void rcv_info_bk(int soc, struct booking *bk)
{
    int i, ret;
    uint32_t conv;
    srand(time(NULL));

    memset(bk, 0, sizeof(struct booking));
    bk->dbk.id_b = rand() % 100000 - 1;

    for (i = 0; i < INFO_SIZE; i++)
    {
        ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nella ricezione delle informazioni sulla prenotazione");
            exit(1);
        }
        if (i == DATE_SIZE)
            bk->dbk.numppl = ntohl(conv);
        else
            bk->date[i] = ntohl(conv);
    }

    ret = recv(soc, (void *)bk->dbk.surname, MAX_DIM_SURNAME, 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione del cognome\n");
        exit(1);
    }
    return;
}

/*
    Dagli id dei tavoli presenti complessivamente nel ristorante, sottrae
    quelli con capacita' < del numero di persone selezionato dall'utente
    e quelli che risultano gia' prenotati.
*/
void build_opz(int *t_opz, int *booked, int numppl)
{
    int k, i, j;
    k = 0; // per scorrere t_opz

    for (i = 0; i < NUM_TABLE; i++)
    { // per scorrere idt_tot
        if ((idt_tot[i] >= numppl))
        {
            j = 0; // per scorrere booked

            /*si controlla se, quello considerato come possibile opzione da proporre,
              non sia gia' occupato*/
            while (j < NUM_TABLE && (booked[j] != -1))
            {
                if (booked[j] == i)
                    break;
                else
                    j++;
            }

            if (j == NUM_TABLE || booked[j] == -1)
                t_opz[k++] = i;
        }
    }
    return;
}

/*
    Recupera le informazioni sui dettagli di ciascun tavolo
*/
void tbdetail(FILE *f, char *buf, int choice, int *t_opz)
{
    int i, j;
    i = 0; // per scorrere t_opz
    j = 0; // come indice per identificare il tavolo
    char *scorri = &buf[0];

    memset(buf, 0, MAX_SIZE);
    rewind(f);
    while (!feof(f) && j < NUM_TABLE)
    {
        fscanf(f, " %[^\n] ", scorri);
        if (choice == -1)
        {
            for (i = 0; i < NUM_TABLE; i++)
            {
                if (t_opz[i] == j)
                {
                    strcat(scorri, "\n");
                    scorri += strlen(scorri);
                    break;
                }
            }
            if (i == NUM_TABLE)
                memset(scorri, 0, strlen(scorri));
        }
        else if (choice == j)
            break;
        j++;
    }
    return;
}

/*
    Trova le disponibilita' per la data selezionata.
*/
bool find_free_tb(struct booking *bk, int scs, char *buf)
{
    int i;
    int tb_booked[NUM_TABLE];

    rcv_info_bk(scs, bk);
    for (i = 0; i < NUM_TABLE; i++)
    {
        bk->t_opz[i] = -1;
        tb_booked[i] = -1;
    }

    find_booked(bk->date, tb_booked, 1);

    // else caso in cui non ci sono prenotazioni
    build_opz(bk->t_opz, tb_booked, bk->dbk.numppl);
    return true;
}

/*
    Invia al client le informazioni sui tavoli disponibili
    per la data da lui selezionata
*/
void send_opz_found(struct booking *bk, int scs, char *buf)
{
    FILE *f;
    memset(buf, 0, MAX_SIZE);
    memset(bk, 0, sizeof(struct booking));

    if (find_free_tb(bk, scs, buf))
    {
        f = fopen("info_tb.txt", "r");
        if (!f)
        {
            perror("Errore nell'apertura del file info_tb.txt\n");
            exit(1);
        }
        tbdetail(f, buf, -1, bk->t_opz);
        fclose(f);
    }
    send_file(scs, buf, 0);
    return;
}

void save_newbk(int scs, struct booking *bk, struct workshift *ws)
{
    FILE *f;
    int ret;
    uint32_t conv;

    ret = recv(scs, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione dell'opzione scelta\n");
        exit(1);
    }
    bk->dbk.opz_choose = ntohl(conv);

    f = fopen("bk_detail.txt", "a+");
    if (!f)
    {
        perror("Errore nell'apertura del file bk_detail.txt\n");
        exit(1);
    }
    fprintf(f, "Id = %-5d IdxT = %-d Surname = %.20s Nppl = %-2d\n", bk->dbk.id_b, bk->t_opz[bk->dbk.opz_choose], bk->dbk.surname, bk->dbk.numppl);
    fclose(f);

    f = fopen("bk_oftheday.txt", "a+");
    if (!f)
    {
        perror("Errore nell'apertura del file bk_oftheday.txt\n");
        exit(1);
    }
    fprintf(f, "Date = %2d-%2d-%2d %2d Id = %-5d\n", bk->date[0], bk->date[1], bk->date[2], bk->date[3], bk->dbk.id_b);
    fclose(f);

    if (bk->date[0] == ws->date[0] && bk->date[1] == ws->date[1])
    {
        if (bk->date[2] == ws->date[2] && bk->date[3] == ws->date[3])
        {
            ws->id_bk[bk->t_opz[bk->dbk.opz_choose]] = bk->dbk.id_b;
        }
    }
    return;
}

void send_ACKnewbk(char *buf, struct booking *bk, int scs)
{
    int ret;
    FILE *f;
    uint32_t conv;

    memset(buf, 0, MAX_SIZE);
    strcpy(buf, "PRENOTAZIONE EFFETTUATA\n");
    send_file(scs, buf, ACK_FOR_BOOK);

    f = fopen("info_tb.txt", "r");
    if (!f)
    {
        perror("Errore nell'apertura del file info_tb.txt\n");
        exit(1);
    }
    tbdetail(f, buf, bk->t_opz[bk->dbk.opz_choose], bk->t_opz);
    fclose(f);
    send_file(scs, buf, 0);

    conv = htonl(bk->dbk.id_b);
    ret = send(scs, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del codice\n");
        exit(1);
    }
    return;
}

void send_ACK_upd_kd_sts(int soc, char *buf)
{
    int ret;
    uint32_t conv;

    ret = 1;
    conv = htonl(ret);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del comando\n");
        exit(1);
    }

    send_file(soc, buf, 0);
    return;
}

void send_ACK_upd_td_sts(int *soc_dev, int cmd, struct order *ord)
{
    int j, ret;
    uint32_t conv;

    for (j = 0; j < NUM_TABLE; j++)
    {
        if (idt_tot[j] == ord->id_tb)
            break;
    }
    j++;

    conv = htonl(cmd);
    ret = send(soc_dev[j], (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del comando\n");
        exit(1);
    }

    send_info_ord(ord, soc_dev[j]);
    return;
}

void send_bk_oftheday(int sc, struct workshift *ws, int idx)
{
    int ret;
    uint32_t conv;

    conv = htonl(ws->id_bk[idx]);
    ret = send(sc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del codice di prenotazione\n");
        exit(1);
    }
    return;
}

void send_menu(int scs, char *buf)
{
    char *scorri = &buf[0];
    FILE *f = fopen("menu.txt", "r");
    if (!f)
    {
        perror("Errore nell'apertura del file menu.txt\n");
        exit(1);
    }

    memset(buf, 0, MAX_SIZE);
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
    send_file(scs, buf, 0);
    return;
}

void rcv_ord(int soc, char *buf, struct order *ord)
{
    int ret;
    uint32_t conv;
    char *scorri = &buf[0];
    srand(time(NULL));
    memset(ord, 0, sizeof(struct order));

    // ricevo il numero di ordine del cliente
    ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione del numero di ordine\n");
        exit(1);
    }
    ord->n_ord = ntohl(conv);

    printf("%s%d\n", "Ordine num. ", ord->n_ord);
    fflush(stdout);

    ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione dell'id del tavolo\n");
        exit(1);
    }
    ord->id_tb = ntohl(conv);

    // ricevo la comanda
    ret = 0;
    rcv_file(soc, buf, &ret);

    do
    {
        if (*scorri == ' ')
            *scorri = '\n';
        else if (*scorri == '-')
            *scorri = ' ';
        scorri++;
    } while (*scorri);

    ord->id_ord = (rand() % 1000) - 1;
    return;
}

void send_ACK_ord(int soc, char *buf, int id_ord)
{
    int ret;
    uint32_t conv;

    printf("%s%d%c\n", "Invio conferma per l'ordine ricevuto dal socket ", soc, '\n');
    fflush(stdout);

    // invio messaggio di ACK
    strcpy(buf, "CR\n");
    send_file(soc, buf, ACK_FOR_ORD);

    // invio id comanda
    conv = htonl(id_ord);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio dell'ID comanda\n");
        exit(1);
    }
    return;
}

int rcv_recap_ord(int soc, char *buf)
{
    int ret;
    // ricevo riepilogo ordini
    printf("%s%d\n", "Ricezione del riepilogo ordini dal socket ", soc);
    fflush(stdout);
    ret = 0;
    rcv_file(soc, buf, &ret);
    printf(buf);
    fflush(stdout);
    return ret;
}

void send_rcp(int soc, struct receipt *rcp)
{
    int ret, i;
    uint32_t conv;

    printf("%s\n", "Calcolo del totale dell'ordine\n");
    fflush(stdout);

    for (i = 0; i < rcp->num_ds_rcp; i++)
    {
        conv = htonl(rcp->tot_ds[i]);
        ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nell'invio del totale per piatto\n");
            exit(1);
        }
    }

    conv = htonl(rcp->tot_ord);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del totale complessivo\n");
        exit(1);
    }
    return;
}

void make_receipt(char *buf, struct receipt *rcp)
{
    int i = 0;
    char *scorri = &buf[0];
    struct dishes menu_ds, ds;
    FILE *f = fopen("menu.txt", "r");
    if (!f)
    {
        perror("Errore nell'apertura del file menu.txt\n");
        exit(1);
    }

    memset(&menu_ds, 0, sizeof(struct dishes));
    memset(&ds, 0, sizeof(struct dishes));

    rewind(f);
    // per ogni piatto presente nel recap vado a calcolare il costo totale
    // e determino la spesa complessiva del cliente
    while (!feof(f))
    {
        if (sscanf(scorri, " %[^ ] %4d%*[^\n] ", ds.id, &ds.quanti) == 2)
        {
            rcp->num_ds_rcp++;
            while (fscanf(f, " %[^ ]%*33c%3d ", menu_ds.id, &menu_ds.quanti) == 2)
            {
                if (!strcmp(menu_ds.id, ds.id))
                {
                    rcp->tot_ds[i] = (ds.quanti * menu_ds.quanti);
                    rcp->tot_ord += rcp->tot_ds[i];
                    i++;
                    break;
                }
            }
            scorri += DIM_ROW_SUMM;
        }
        else
            break;
    }
    fclose(f);
    return;
}

void save_rcp(int id_tb, char *buf, struct receipt *rcp)
{
    int i = 0;
    char dish[DIM_ROW_SUMM];
    char *scorri = &buf[0];
    FILE *f = fopen("recap_rcp.txt", "a+");
    if (!f)
    {
        perror("Errore nell'apertura del file recap_rcp.txt\n");
        exit(1);
    }

    memset(dish, 0, DIM_ROW_SUMM);
    fprintf(f, "%s%d\n", "TAVOLO T", idt_tot[id_tb]);
    printf("%s%d\n", "TAVOLO T", idt_tot[id_tb]);
    fflush(stdout);

    while (sscanf(scorri, "%[^\n] ", dish) == 1)
    {
        while (!rcp->tot_ds[i])
            i++;

        fprintf(f, "%s %d\n", dish, rcp->tot_ds[i]);
        printf("%s %d\n", dish, rcp->tot_ds[i++]);
        fflush(stdout);
        scorri += (strlen(dish) + 1);
        strcpy(dish, "");
    }

    fprintf(f, "%s%d\n", "Totale: ", rcp->tot_ord);
    fclose(f);
    printf("%s%d\n", "Totale: ", rcp->tot_ord);
    fflush(stdout);
    return;
}

void notify_to_kd(int *soc_dev, int pending)
{
    int i, ret, choice;
    uint32_t conv;

    for (i = 4; i < 6; i++)
    {
        if (soc_dev[i])
        {
            choice = -1;
            conv = htonl(choice);
            ret = send(soc_dev[i], (void *)&conv, sizeof(uint32_t), 0);
            if (ret < 0)
            {
                perror("Errore nell'invio del comando\n");
                exit(1);
            }

            conv = htonl(pending);
            ret = send(soc_dev[i], (void *)&conv, sizeof(uint32_t), 0);
            if (ret < 0)
            {
                perror("Errore nell'invio del numero di ordini pendenti\n");
                exit(1);
            }
        }
    }
    return;
}

int main(int argc, char *argv[])
{
    int i, ret, choice, j;
    uint32_t conv;
    char buf[MAX_SIZE];
    struct booking bk;
    struct order ord;
    struct workshift ws;
    struct receipt rcp;
    struct server_r_info r;

    signal(SIGINT, handler);
    // signal(SIGTSTP, handler);

    if (argc == 2)
    {
        port = atoi(argv[1]);
        if (!check_port(port, 0))
            ;
        {
            perror("Porta errata\n");
            exit(1);
        }
    }

    start(&r, &ws);
    memset(&ord, 0, sizeof(struct order));
    memset(buf, 0, MAX_SIZE);

    for (;;)
    {
        r.ser.read_fds = r.ser.master;

        ret = select(r.ser.max_fd + 1, &r.ser.read_fds, NULL, NULL, NULL);
        if (ret < 0)
        {
            perror("Server non attivo\n");
            exit(1);
        }

        for (i = 0; i <= r.ser.max_fd; i++)
        {
            choice = 0;
            memset(buf, 0, MAX_SIZE);

            if (FD_ISSET(i, &r.ser.read_fds))
            {
                if (i == STDIN_FILENO)
                {
                    if (read(STDIN_FILENO, buf, MAX_SIZE) > 0)
                    {
                        if (!strcmp(buf, "stop\n"))
                        {
                            stop(&r);
                            return 0;
                        }
                        else
                        {
                            check_input(buf);
                        }
                    }
                }
                // richiesta di connessione al socket di ascolto
                else if (i == r.ser.soc)
                    accept_new_conn(&r);
                else if (i == r.soc_dev[0])
                {
                    ret = recv(r.soc_dev[0], (void *)&conv, sizeof(uint32_t), 0);
                    if (ret < 0)
                    {
                        perror("Socket chiuso");
                        r.soc_dev[0] = 0;
                        exit(1);
                    }

                    choice = ntohl(conv);
                    switch (choice)
                    {
                    case 1:
                        printf("%s%d\n", "Elaborazione richiesta prenotazione per socket ", r.soc_dev[0]);
                        fflush(stdout);
                        send_opz_found(&bk, r.soc_dev[0], buf);
                        break;
                    case 2:
                        save_newbk(r.soc_dev[0], &bk, &ws);
                        printf("%s%d\n", "Aggiunta nuova prenotazione, id: ", bk.dbk.id_b);
                        fflush(stdout);
                        send_ACKnewbk(buf, &bk, r.soc_dev[0]);
                        break;
                    case 3:
                        close(i);
                        printf("%s%d\n", "Chiusura del socket ", r.soc_dev[0]);
                        fflush(stdout);
                        FD_CLR(i, &r.ser.master);
                        r.soc_dev[0] = 0;
                        break;
                    }
                }
                else if (i == r.soc_dev[1] || i == r.soc_dev[2] || i == r.soc_dev[3])
                {
                    for (j = 1; j < 4; j++)
                    {
                        if (i == r.soc_dev[j])
                        {
                            j--;
                            break;
                        }
                    }
                    ret = recv(i, (void *)&conv, sizeof(uint32_t), 0);
                    if (ret < 0)
                    {
                        perror("Socket chiuso\n");
                        exit(1);
                    }

                    choice = ntohl(conv);
                    switch (choice)
                    {
                    case -1:
                        send_bk_oftheday(i, &ws, 0);
                        printf("%s%d\n", "Richiesto codice prenotazione dal tbdevice T", idt_tot[0]);
                        fflush(stdout);
                        break;
                    case 2:
                        printf("%s%d\n", "Invio menu al socket ", i);
                        fflush(stdout);
                        send_menu(i, buf);
                        break;
                    case 3:
                        printf("%s%d%c\n", "Ricevuta una comanda dal socket ", i, '\n');
                        fflush(stdout);
                        rcv_ord(i, buf, &ord);
                        save_ord(buf, &ord);
                        send_ACK_ord(i, buf, ord.id_ord);
                        ws.tot_pnd_ord++;

                        printf("%s%d\n\n", "Totale ordini pendenti: ", ws.tot_pnd_ord);
                        fflush(stdout);
                        notify_to_kd(r.soc_dev, ws.tot_pnd_ord);
                        break;
                    case 4:
                        memset(&rcp, 0, sizeof(struct receipt));
                        if (rcv_recap_ord(i, buf))
                        {
                            make_receipt(buf, &rcp);
                            send_rcp(i, &rcp);
                            save_rcp(0, buf, &rcp);
                        }
                        else
                        {
                            printf("%s%d\n", "Non sono presenti ordinazioni per il socket ", i);
                            fflush(stdout);
                        }
                        close(i);
                        printf("%s%d\n", "Chiusura del socket ", i);
                        fflush(stdout);
                        FD_CLR(i, &r.ser.master);
                        j++;
                        r.soc_dev[j] = 0;
                        break;
                    default:
                        break;
                    }
                }
                else if (i == r.soc_dev[4] || i == r.soc_dev[5])
                {
                    ret = recv(i, (void *)&conv, sizeof(uint32_t), 0);
                    if (ret < 0)
                    {
                        perror("Errore nella ricezione dei dati\n");
                        exit(1);
                    }

                    choice = ntohl(conv);
                    switch (choice)
                    {
                    case -1:
                        conv = htonl(ws.tot_pnd_ord);
                        ret = send(i, (void *)&conv, sizeof(uint32_t), 0);
                        if (ret < 0)
                        {
                            perror("Errore nell'invio del numero di ordini pendenti\n");
                            exit(1);
                        }
                        break;
                    case 1:
                        if (ws.tot_pnd_ord)
                        {
                            find_oldest_pnd(buf, &ord);
                            printf("%s\n", buf);
                            fflush(stdout);
                            if (ord.sts == '1')
                            {
                                ws.tot_pnd_ord--;
                                // invio al tb device l'aggiornamento dello stato
                                ord.sts = '0';
                                send_ACK_upd_td_sts(r.soc_dev, 1, &ord);
                                send_ACK_upd_kd_sts(i, buf);
                                notify_to_kd(r.soc_dev, ws.tot_pnd_ord);
                                printf("%s%d\n", "Ordini pendenti restanti ", ws.tot_pnd_ord);
                                fflush(stdout);
                            }
                        }
                        else
                        {
                            printf("Non ci sono ordini pendenti\n");
                            fflush(stdout);
                        }
                        break;
                    case 3:
                        memset(&ord, 0, sizeof(struct order));
                        rcv_info_ord(&ord, i);
                        update_sts("ord_oftheday.txt", '2', &ord);
                        if (!ord.id_ord)
                        {
                            printf("Impossibile aggiornare la comanda\n");
                            fflush(stdout);
                        }
                        else
                        {
                            send_ACK_upd_td_sts(r.soc_dev, 2, &ord);
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
