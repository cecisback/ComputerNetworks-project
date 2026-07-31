#include "header_server.h"

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
    Dati in ingresso:
    - la data selezionata
    - un array della dimensione del numero di tavoli presenti in sala
    
    modifica il contenuto dell'array in input così da salvare in corrispondenza di ciascun 
    tavolo (indice dell'array) l'ID della prenotazione presente per la data cercata
*/
void find_bookings(int *date, int *reservation_tb)
{
    int datefound[DATE_SIZE];

    int id_booked_tb, id_reservation, matches = 0;

    memset(reservation_tb, -1, sizeof((*reservation_tb)*NUM_TABLE));

    FILE *f;

    f = fopen("bk_of_the_day.txt", "r");
    if (!f){
        return;
    }

    rewind(f);
    while (!feof(f))
    {
        /*Formato file: Date = 20-6-23 0 IdxT 0 Id 26429
        dove il primo 0 corrisponde al turno: 0 pranzo, 1 cena e il secondo 0 corrisponde all'ID del tavolo prenotato.
        L'ultimo ID è quello della prenotazione.
        */ 
        if (fscanf(f, " %*7c%d%*c%d%*c%d%*c%d%*6c%d%*4c%d ", &datefound[0], &datefound[1], &datefound[2], &datefound[3], &id_booked_tb, &id_reservation) == 6)
        {
            matches = 0;
            for (int i = 0; i < DATE_SIZE; i++)
            {
                if (datefound[i] == date[i]){
                    matches += 1;
                }else{
                    break;
                }
            }

            /* se ha trovato una prenotazione nella lista con stessa data
            e stessa fascia oraria richiesta ne salva l'identificatore in corrispondenza
            del tavolo prenotato */
            if (matches == DATE_SIZE){
                if (reservation_tb[id_booked_tb] == -1){
                    reservation_tb[id_booked_tb] = id_reservation;
                }else{
                    perror("Errore nell'inserimento delle prenotazioni: presenti multiple prenotazioni per uno stesso tavolo e nello stesso turno di lavoro\n");
                    exit(1);
                }
            }
        }else{
            perror("Formato del contenuto presente nel file non valido\n");
            exit(1);
        }
    }
    fclose(f);

    return;
}

/*
    Salva in una struttura dati comune, per ciascuna porta in ascolto, il socket creato server side 
    dopo l'accettazione della richiesta di binding giunta dal client.
*/

/*
    Controlla che la porta associata dall'utente al momento
    della chiamata al programma sia effettivamente corretta
*/
bool check_port_server(int port)
{
    if (port == SERVER_PORT)
        return true;

    return false;
}

void save_info_connection(int soc, int port, int *soc_dev)
{
    int i;

    switch (port)
    {
    case CLIENT_PORT:
        i = 0;
        break;
    case TABLE_1_PORT:
        i = 1;
        break;
    case TABLE_23_PORT:
        i = 2;
        break;
    case TABLE_4_PORT:
        i = 3;
        break;
    case KTC_1_PORT:
        i = 4;
        break;
    case KTC_2_PORT:
        i = 5;
        break;
    };

    soc_dev[i] = soc;

    printf("%s\n", "Salvataggio delle informazioni sulla nuova connessione...");
    fflush(stdout);
    return;
}

/*
    Accetta una nuova connessione server side e aggiorna i parametri della struttura dati
    server_r_info per porsi in ascolto di successivi comandi giunti dal socket client.
    Viene inoltre salvato il socket del client con il quale si è aperta la nuova connessione.
*/
void accept_new_connection(struct Server_r_info *r)
{
    struct sockaddr_in cl_address;
    int len, soc, port;

    len = sizeof(cl_address);
    memset(&cl_address, 0, sizeof(struct sockaddr_in));
  
    soc = accept(r->server_IO.soc, (struct sockaddr *)&cl_address, (socklen_t *)&len);
    port = ntohs(cl_address.sin_port);
    printf("%s %d\n", "Nuova connessione su porta:", port);
    fflush(stdout);

    save_info_connection(soc, port, r->sockets_server);
    FD_SET(soc, &r->server_IO.master);

    if (soc > r->server_IO.max_fd)
        r->server_IO.max_fd = soc;
    return;
}

/* ------------------------------------------------------------------------------------------ */
/*                    UTILITIES FOR INITIALIZING SERVER SIDE DATA STRUCTURES                  */
/* ------------------------------------------------------------------------------------------ */

/*
    Creazione del file contenente le informazioni relative ai tavoli
    presenti in sala.
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
    Creazione del file contenente il menù.
*/
void init_menu()
{
    FILE *f;
    f = fopen("menu.txt", "w");
    if (!f)
    {
        perror("Errore nella scrittura del file menù.\n");
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
    Inizializzazione della struttura dati "workshift" con le informazioni relative
    alla data odierna, al turno di lavoro attuale e alle prenotazioni previste per
    ciascun tavolo per il turno di lavoro considerato.
    Sono previsti due turni di lavoro: uno al pranzo e l'altro a cena.
*/
void init_info_workshift(struct Workshift *ws)
{
    struct tm *timeinfo = current_date();
    ws->date[0] = timeinfo->tm_mday;
    ws->date[1] = timeinfo->tm_mon + 1;
    ws->date[2] = timeinfo->tm_year - 100;
    ws->date[3] = 0; 
    find_bookings(ws->date, ws->reservation_ID);
    return;
}

/* ------------------------------------------------------------------------------------------ */
/*                    UTILITIES FOR SERVER STARTUP AND SHUTDOWN                               */
/* ------------------------------------------------------------------------------------------ */

/*
    Avvio del server.
    Questa funzione ha come obiettivo l'inizializzazione di importanti strutture dati 
    che verranno usate lato server per lo svolgimento delle funzionalità previste da sistema e 
    l'interazione con i processi client:
    - creazione del file contenente identificativo e dettagli dei tavoli presenti in sala;
    - creazione del file contenente il menù;
    - Server_r_info, dettagli in header_server.h;
    - Workshift, per tenere traccia del turno di lavoro corrente;
*/
void start_server(struct Server_r_info *r, struct Workshift *ws)
{
    memset(r, 0, sizeof(struct Server_r_info));
    memset(ws, 0, sizeof(struct Workshift));

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
    r->server_IO.soc = bind_to_soc(&r->myaddr);
    listen(r->server_IO.soc, 10);
    printf("%s\n", "Creazione del socket di ascolto..");
    fflush(stdout);

    init_stream_IO(&r->server_IO);
    print_cmd();
    return;
}

/*
    Spegne il server e lo notifica a tutti i restanti dispositivi
    connessi.
*/
void stop_server(struct Server_r_info *r)
{
    int i, ret;
    uint32_t conv;

    remove("ord_oftheday.txt");
    remove("menu.txt");
    remove("info_tb.txt");

    printf("Eliminazione del file ord_oftheday.txt...\n");
    printf("Eliminazione del file menu.txt...\n");
    printf("Eliminazione del file info_tb.txt...\n");
    fflush(stdout);

    for (i = 0; i < N_PORT; i++)
    {
        if (r->sockets_server[i])
        {
            conv = htonl(STOP_SERVER_CMD);
            ret = send(r->sockets_server[i], (void *)&conv, sizeof(uint32_t), 0);
            if (ret < 0)
            {
                perror("Errore nell'invio della richiesta di spegnimento server\n");
                exit(1);
            }

            close(r->sockets_server[i]);
            FD_CLR(r->sockets_server[i], &r->server_IO.master);
            printf("%s%d%s\n", "Socket n. ", r->sockets_server[i], " chiuso\n");
            fflush(stdout);
        }
    }

    FD_ZERO(&r->server_IO.read_fds);
    FD_ZERO(&r->server_IO.write_fds);
    r->server_IO.max_fd = 0;
    r->server_IO.soc = 0;

    return;
}

/* ------------------------------------------------------------------------------------------ */
/*                UTILITIES FOR HANDLING COMMANDS ISSUED BY THE SERVER ITSELF                 */
/* ------------------------------------------------------------------------------------------ */

/*
    Stampa a video lo stato in formato stringa.
*/
void show_status_order(int status)
{
    switch (status)
    {
    case 0:
        printf("<in attesa>\n");
        break;
    case 1:
        printf("<in preparazione>\n");
        break;
    case 2:
        printf("<in servizio>\n");
        break;
    }
    fflush(stdout);
    return;
}

void show_order_list(int table_ID, int status, char *buf)
{
    bool found = false;
    struct Order ord;

    FILE *f = fopen("ord_oftheday.txt", "r");
    if (!f)
    {
        printf("Non ci sono prenotazioni per questo tavolo\n");
        return;
    }

    memset(&ord, 0, sizeof(struct Order));

    rewind(f);
    while (!feof(f))
    {
        memset(buf, 0, MAX_SIZE);

        if (fscanf(f, " %[^\n] ", buf))
        {
            if (sscanf(buf, "ID:%*3c com%*d T%d STATUS = %d\n", &ord.table_ID, &ord.status) == 2)
            {   
                /*
                    Mostra tutti gli elementi salvati nel file ord_oftheday.txt
                */
                if(table_ID == -1 && status == -1){
                    buf += DIM_ROW_ORDER;
                    strcpy(buf, "");
                    printf("%s ", buf);
                    fflush(stdout);
                    show_status_order(status);
                }
                else
                {
                    if (ord.status == status)
                    {
                        strcpy(&buf[DIM_ROW_ORDER], "");
                        found = true;
                    }
                    else if (ord.table_ID == table_ID)
                    {
                        strcpy(&buf[12], "");
                        show_status_order(ord.status);
                        found = true;
                    }
                    else
                        found = false;
                }
            }

            if (found == true)
            {
                printf("%s\n", buf);
                fflush(stdout);
            }
            else
            {
                if(table_ID == -1 && status == -1){
                    printf("%s\n", buf);
                    fflush(stdout);
                }
            }
        }
    }
    fclose(f);
    return;
}


/*
    Controllo comandi in input del server
*/
void check_input(char *buf)
{
    int table_ID, i;

    if (!strcmp(buf, "status p\n"))
        show_order_list(0, 1, buf);
    else if (!strcmp(buf, "status a\n"))
        show_order_list(0, 0, buf);
    else if (!strcmp(buf, "status s\n"))
        show_order_list(0, 2, buf);
    else if (!strcmp(buf, "stat\n"))
    {
        show_order_list(-1,-1,buf);
    }
    else if (sscanf(buf, "stat T%d\n", &table_ID))
    {
        for (i = 0; i < NUM_TABLE; i++)
        {
            if (table_ID == tables_ID_list[i])
                break;
        }

        if (i < NUM_TABLE)
        {
            show_order_list(table_ID, -1, buf);
        }
        else
        {
            printf("Tavolo inserito non presente\n");
            fflush(stdout);
        }
    }
    return;
}

/* ----------------------------------------------------------------------------------------- */
/*                      UTILITIES FOR CLIENT VALIDATION REQUESTS                             */
/* ----------------------------------------------------------------------------------------- */

/*
    Riceve dal client il codice di prenotazione e si accerta che corrisponda al codice
    associato alla prenotazione prevista per quel tavolo nella data e fascia oraria corrente.
    Queste informazioni sono state recuperate dal file bk_of_the_day.txt, accessibile server side,
    e con il quale è stata inizializzata la struttura dati workshift (anch'essa utilizzabile 
    esclusivamente server side).
*/
int auth_request_client(int sc, struct Workshift *ws)
{
    int ret, id_b;
    uint32_t conv;

    ret = recv(sc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione del codice di prenotazione\n");
        exit(1);
    }
    id_b = ntohl(conv);

    if (!id_b){
        printf("%s\n", "Codice di prenotazione ricevuto non valido o non corrispondente ad un valore ammissibile");
        fflush(stdout);
        return AUTHENTICATION_FAILED_CODE;
    }else if (id_b != *(ws->reservation_ID)){
        printf("%s\n", "I codici di prenotazione non corrispondono");
        fflush(stdout);
        return AUTHENTICATION_FAILED_CODE;
    }

    printf("%s\n","Autenticazione avvenuta con successo");
    return AUTHENTICATION_ACK;
}

void send_ACK_validation_request(int soc, bool outcome)
{
    int ret;
    uint32_t conv;

    printf("%s%d\n", "Invio responso validazione richiesta del client al socket ", soc);
    fflush(stdout);

    int cod = AUTHENTICATION_FAILED_CODE;
    if (outcome == false){
        cod = AUTHENTICATION_ACK;
    }

    conv = htonl(cod);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del responso validazione richiesta\n");
        exit(1);
    }
    return;
}

/* ----------------------------------------------------------------------------------------- */
/*                             UTILITIES FOR MAKING RECEIPTS                                 */
/* ----------------------------------------------------------------------------------------- */

void make_receipt(char *buf, struct Receipt *rcp)
{
    int i = 0;
    char *recap_ord = &buf[0];
    struct Dish menu_ds, ds;
    
    FILE *f = fopen("menu.txt", "r");
    if (!f)
    {
        perror("Errore nell'apertura del file menu.txt\n");
        exit(1);
    }

    memset(&menu_ds, 0, sizeof(struct Dish));
    memset(&ds, 0, sizeof(struct Dish));

    rewind(f);
    /*
        Per ogni piatto ordinato, presente nella lista finale con rispettivo
        quantitativo, vado a calcolare il costo totale e determino la spesa 
        complessiva del cliente
    */

    while (!feof(f))
    {
        if (sscanf(recap_ord, " %[^ ] %4d%*[^\n] ", ds.dish_ID, &ds.quantity) == 2)
        {
            rcp->num_orders++;
            while (fscanf(f, " %[^ ]%*33c%3d ", menu_ds.dish_ID, &menu_ds.price) == 2)
            {
                if (!strcmp(menu_ds.dish_ID, ds.dish_ID))
                {
                    rcp->subtotals[i] = (ds.quantity * menu_ds.price);
                    rcp->total += rcp->subtotals[i];
                    i++;
                    break;
                }
            }
            recap_ord += DIM_ROW_ORDER;
        }
        else
            break;
    }
    fclose(f);
    return;
}

void save_receipt(int id_tb, char *buf, struct Receipt *rcp)
{
    int i = 0;
    char dish[DIM_ROW_ORDER];
    char *recap_ord = &buf[0];
    FILE *f = fopen("recap_rcp.txt", "a+");
    if (!f)
    {
        perror("Errore nell'apertura del file recap_rcp.txt\n");
        exit(1);
    }

    memset(dish, 0, DIM_ROW_ORDER);
    fprintf(f, "%s%d\n", "TAVOLO T", tables_ID_list[id_tb]);
    printf("%s%d\n", "TAVOLO T", tables_ID_list[id_tb]);
    fflush(stdout);

    while (sscanf(recap_ord, "%[^\n] ", dish) == 1)
    {
        while (!rcp->subtotals[i])
            i++;

        fprintf(f, "%s %d\n", dish, rcp->subtotals[i]);
        printf("%s %d\n", dish, rcp->subtotals[i++]);
        fflush(stdout);
        recap_ord += (strlen(dish) + 1);
        strcpy(dish, "");
    }

    fprintf(f, "%s%d\n", "Totale: ", rcp->total);
    fclose(f);
    printf("%s%d\n", "Totale: ", rcp->total);
    fflush(stdout);
    return;
}

void send_receipt(int soc, struct Receipt *rcp)
{
    int ret, i;
    uint32_t conv;

    printf("%s\n", "Calcolo del totale dell'ordine\n");
    fflush(stdout);

    for (i = 0; i < rcp->num_orders; i++)
    {
        conv = htonl(rcp->subtotals[i]);
        ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nell'invio del totale per piatto\n");
            exit(1);
        }
    }

    conv = htonl(rcp->total);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del totale complessivo\n");
        exit(1);
    }
    return;
}

/* ------------------------------------------------------------------------------------------ */
/*                    UTILITIES FOR EXECUTING COMMANDS RECEIVED FROM TABLES                   */
/* -------------------------------------------------------------------------------------------*/
void send_menu(int scs, char *buf)
{
    FILE *f = fopen("menu.txt", "r");
    if (!f)
    {
        perror("Errore nell'apertura del file menu.txt\n");
        exit(1);
    }

    memset(buf, 0, MAX_SIZE);

    char *stream_reader = &buf[0];

    rewind(f);
    while (!feof(f))
    {
        if (fscanf(f, " %[^\n] ", stream_reader))
        {
            strcat(stream_reader, "\n");
            stream_reader += strlen(stream_reader);
        }
    }

    fclose(f);
    send_file(scs, buf, 0);
    return;
}

void send_info_order_to_tb(int *soc_dev, int command, struct Order *ord)
{
    int i, ret;
    uint32_t conv;

    for (i = 0; i < NUM_TABLE; i++)
    {
        if (tables_ID_list[i] == ord->table_ID)
            break;
    }
    i++;

    conv = htonl(command);
    ret = send(soc_dev[i], (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del comando\n");
        exit(1);
    }

    send_info_order(ord, soc_dev[i]);
    return;
}

/* ----------------------------------------------------------------------------------------------*/
/*                           UTILITIES FOR HANDLING THE RESERVATION PHASE                        */
/* ----------------------------------------------------------------------------------------------*/

/*
    Riceve le informazioni inserite dall'utente per la prenotazione,
    inclusi data e numero di persone, e genera randomicamente un ID di prenotazione univoco.
*/
void retrieve_reservation_info(int soc, struct Reservation_request *req)
{
    int i, ret;
    uint32_t conv;

    srand(time(NULL));

    memset(req, 0, sizeof(struct Reservation_request));

    req->reservation_info.id = rand() % 100000 - 1;

    for (i = 0; i < INFO_SIZE; i++)
    {
        ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nella ricezione delle informazioni sulla prenotazione");
            exit(1);
        }
        if (i == DATE_SIZE)
            req->reservation_info.num_guests = ntohl(conv);
        else
            req->date[i] = ntohl(conv);
    }

    ret = recv(soc, (void *)req->reservation_info.surname, SURNAME_SIZE, 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione del cognome\n");
        exit(1);
    }
    return;
}

/*
    Dagli ID dei tavoli presenti complessivamente nel ristorante, sottrae
    quelli con capacita' < del numero di persone selezionato dall'utente
    e quelli che risultano gia' prenotati.
*/
void generate_reservation_proposal(int *available_tables, int *not_available, int guests)
{
    int k, i;
    k = 0;

    memset(available_tables, 0, sizeof((*available_tables)*NUM_TABLE));

    for (i = 0; i < NUM_TABLE; i++)
    { 
        // Per ciascun tavolo presente in sala
        if ((tables_ID_list[i] >= guests))
        {
            /* controlla se, il tavolo considerato come possibile opzione da proporre, non sia gia' occupato*/
            if (not_available[i] == -1){
                available_tables[k++] = i;
            }
        }
    }
    return;
}

/*
    Trova le disponibilita' per la data selezionata.
*/
bool find_available_tables(struct Reservation_request *bk, int socket)
{
    int i;
    int tb_booked[NUM_TABLE];

    retrieve_reservation_info(socket, bk);
    for (i = 0; i < NUM_TABLE; i++)
    {
        bk->available_table_IDs[i] = -1;
        tb_booked[i] = -1;
    }

    find_bookings(bk->date, tb_booked);

    generate_reservation_proposal(bk->available_table_IDs, tb_booked, bk->reservation_info.num_guests);
    return true;
}

/*
    Recupera le informazioni sui dettagli del tavolo selezionato dall'utente
    nella parte finale della prenotazione o sui tavoli proposti dal sistema.
*/
void retrieve_table_details(FILE *f, char *buf, int idx_table, int *available_tables)
{
    int i, j;
    i = 0; // per scorrere available_tables
    j = 0; // come indice per identificare il tavolo
    char *stream_reader = &buf[0];

    memset(buf, 0, MAX_SIZE);

    rewind(f);
    while (!feof(f) && j < NUM_TABLE)
    {
        fscanf(f, " %[^\n] ", stream_reader);
        if (idx_table == -1)
        {
            for (i = 0; i < NUM_TABLE; i++)
            {
                if (available_tables[i] == j)
                {
                    strcat(stream_reader, "\n");
                    stream_reader += strlen(stream_reader);
                    break;
                }
            }
            if (i == NUM_TABLE)
                memset(stream_reader, 0, strlen(stream_reader));
        }else if (idx_table == j)
            break;
        j++;
    }
    return;
}

/*
    Invia al client le informazioni sui tavoli disponibili
    per la data da lui selezionata
*/
void send_reservation_proposal(struct Reservation_request *bk, int socket, char *buf)
{
    FILE *f;
    memset(buf, 0, MAX_SIZE);
    memset(bk, 0, sizeof(struct Reservation_request));

    if (find_available_tables(bk, socket))
    {
        f = fopen("info_tb.txt", "r");
        if (!f)
        {
            perror("Errore nell'apertura del file info_tb.txt\n");
            exit(1);
        }

        retrieve_table_details(f, buf, -1, bk->available_table_IDs);
        fclose(f);
    }

    send_file(socket, buf, 0);
    return;
}

void send_ACK_new_reservation(char *buf, struct Reservation_request *bk, int socket)
{
    int ret;
    FILE *f;
    uint32_t conv;

    conv = htonl(RESERVATION_ACK);
    ret = send(socket, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio della conferma avvenuta registrazione della prenotazione\n");
        exit(1);
    }

    f = fopen("info_tb.txt", "r");
    if (!f)
    {
        perror("Errore nell'apertura del file info_tb.txt\n");
        exit(1);
    }

    retrieve_table_details(f, buf, bk->available_table_IDs[bk->reservation_info.selected_table], bk->available_table_IDs);
    fclose(f);
    send_file(socket, buf, 0);

    conv = htonl(bk->reservation_info.id);
    ret = send(socket, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del codice di prenotazione\n");
        exit(1);
    }
    return;
}

void save_new_reservation(int socket, struct Reservation_request *bk, struct Workshift *ws)
{
    FILE *f;
    int ret;
    uint32_t conv;

    ret = recv(socket, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione del tavolo scelto\n");
        exit(1);
    }
    bk->reservation_info.selected_table = ntohl(conv);

    f = fopen("reservations_detail.txt", "a+");
    if (!f)
    {
        perror("Errore nell'apertura del file reservations_detail.txt\n");
        exit(1);
    }
    fprintf(f, "Id = %-5d IdxT = %-d Surname = %.20s Nppl = %-2d\n", bk->reservation_info.id, bk->available_table_IDs[bk->reservation_info.selected_table], bk->reservation_info.surname, bk->reservation_info.num_guests);
    fclose(f);

    f = fopen("bk_of_the_day.txt", "a+");
    if (!f)
    {
        perror("Errore nell'apertura del file bk_of_the_day.txt\n");
        exit(1);
    }
    fprintf(f, "Date = %2d-%2d-%2d %2d Id = %-5d\n", bk->date[0], bk->date[1], bk->date[2], bk->date[3], bk->reservation_info.id);
    fclose(f);

    if (bk->date[0] == ws->date[0] && bk->date[1] == ws->date[1])
    {
        if (bk->date[2] == ws->date[2] && bk->date[3] == ws->date[3])
        {
            ws->reservation_ID[bk->available_table_IDs[bk->reservation_info.selected_table]] = bk->reservation_info.id;
        }
    }
    return;
}

/* -------------------------------------------------------------------------------------*/
/*                           UTILITIES FOR HANDLING ORDERS                              */
/* -------------------------------------------------------------------------------------*/

void send_order_to_ktc(int soc, char *ord)
{
    int ret;
    uint32_t conv;

    conv = htonl(UPDATE_STATUS_ORD_1);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del comando\n");
        exit(1);
    }

    send_file(soc, ord, 0);
    return;
}

/*
    Controlla che tutti gli ordini siano stati serviti.
*/
bool check_status_order()
{
    int status;

    FILE *f = fopen("ord_oftheday.txt", "r");
    if (!f)
    {
        perror("Errore nell'apertura del file\n");
        exit(1);
    }

    rewind(f);
    while (!feof(f))
    {
        if (fscanf(f, "%*25c%d", &status))
        {
            if (status != 2)
            {
                fclose(f);
                return false;
            }
        }
    }
    fclose(f);
    return true;
}

void receive_order(int soc, char *buf, struct Order *ord)
{
    int ret;
    uint32_t conv;
    char *stream_reader = &buf[0];

    srand(time(NULL));

    memset(ord, 0, sizeof(struct Order));

    // ricevo il numero di ordini effettuati dal cliente
    ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione del numero di ordine\n");
        exit(1);
    }
    ord->order_counter = ntohl(conv);

    printf("%s%d\n", "Ordine num. ", ord->order_counter);
    fflush(stdout);

    // ricevo la comanda
    ret = 0;
    receive_file(soc, buf, &ret);

    // Modifico il formato dell'ordine ricevuto
    do
    {
        if (*stream_reader == ' ')
            *stream_reader = '\n';
        else if (*stream_reader == '-')
            *stream_reader = ' ';
        stream_reader++;
    } while (*stream_reader);

    ord->order_ID = (rand() % 1000) - 1;
    return;
}

void send_ACK_order(int soc, int id_ord)
{
    int ret;
    uint32_t conv;

    printf("%s%d\n", "Invio conferma per l'ordine ricevuto dal socket ", soc);
    fflush(stdout);

    conv = htonl(ORDER_ACK);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio della conferma ricezione ordine\n");
        exit(1);
    }

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

int receive_recap_order(int soc, char *buf)
{
    int ret;
    // ricevo riepilogo ordini
    printf("%s%d\n", "Ricezione del riepilogo ordini dal socket ", soc);
    fflush(stdout);

    ret = 0;
    receive_file(soc, buf, &ret);
    printf("%s\n",buf);
    fflush(stdout);
    return ret;
}

void notify_n_remaining_pending_orders(int *soc_dev, int pending)
{
    int i, ret;
    uint32_t conv;

    for (i = IDX_FIRST_KTC; i < IDX_LAST_KTC; i++)
    {
        if (soc_dev[i])
        {
            conv = htonl(START_KC_DEVICE);
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

void pending_to_preparing_status_transition_completed(struct Order ord, char *stream_reader, FILE *f){
    strcpy(stream_reader, "");
    strcat(stream_reader++, "\n");
    while (fscanf(f, " %[^\n] ", stream_reader) == 1)
    {
        if (strncmp(stream_reader, "ID:", 3))
        {
            strcat(stream_reader, "\n");
            stream_reader += strlen(stream_reader);
        }
        else
        {
            strcpy(stream_reader, "");
            break;
        }
    }

    printf("Ordine %d in preparazione\n", ord.order_ID);
    fflush(stdout);
}

void find_oldest_pending_order(char *buf, struct Order *ord)
{
    fpos_t pos;
    char *stream_reader = &buf[15];

    FILE *f = fopen("ord_oftheday.txt", "r+");
    if (!f)
    {
        printf("Non sono presenti ordinazioni\n");
        fflush(stdout);
        return;
    }

    memset(ord, 0, sizeof(struct Order));

    rewind(f);
    while (!feof(f))
    {
        memset(buf, 0, MAX_SIZE);

        if (fscanf(f, " %[^\n] ", buf))
        {
            if (!strncmp(buf, "ID:", 3))
            {
                sscanf(buf, "ID:%3d com%2d T%2d STATUS = %d\n", &ord->order_ID, &ord->order_counter, &ord->table_ID, &ord->status);
                if (ord->status == 0)
                {
                    fgetpos(f, &pos);
                    pos.__pos -= 2;
                    fsetpos(f, &pos);
                    ord->status = 1;
                    fputc(ord->status, f);
                    break;
                }
            }
        }
    }

    if (ord->status == 1)
    {
        pending_to_preparing_status_transition_completed(*ord, stream_reader, f);
    }
    return;
}

/*
    Salva l'ordinazione ricevuta
*/
void store_waiting_order(char *buf, struct Order *ord)
{
    FILE *f = fopen("ord_oftheday.txt", "a+");
    if (!f)
    {
        perror("Errore nell'apertura del file ord.txt\n");
        exit(1);
    }
    ord->status = 0;
    fprintf(f, "ID:%-3d com%-2d T%-2d STATUS = %c\n%s", ord->order_ID, ord->order_counter, tables_ID_list[ord->table_ID], ord->status, buf);
    fclose(f);
    printf("ID:%-3d com%-2d T%-2d ", ord->order_ID, ord->order_counter, tables_ID_list[ord->table_ID]);
    show_status_order(ord->status);
    printf("%s", buf);
    fflush(stdout);
    return;
}