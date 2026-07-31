#include "client_side/communication_utilities/soc_client_utils.c"
#include "client_side/headers/header_table_device.h"
#include "client_side/headers/header_order.h"
#include "client_side/order_shared_utils.c"

void print_cmd()
{
    printf("%s", "1) help --> mostra i dettagli dei comandi\n");
    printf("%s", "2) menu --> mostra il menu dei piatti\n");
    printf("%s", "3) comanda --> invia una comanda\n");
    printf("%s", "4) conto --> chiede il conto\n\n");
    fflush(stdout);
    return;
}

void cleanup_table_device(struct Socket_stream_IO *tb)
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
    Prende il codice di prenotazione inserito dall'utente e lo confronta
    con il codice di prenotazione previsto per quel tavolo, per
    la data e il turno di lavoro correnti.
*/
void enter_reservation_id(char *buf, int reservation_id)
{
    int id;
    char last_typed;

    while (1)
    {
        memset(buf, 0, MAX_SIZE);
        id = 0;

        if (fgets(buf, MAX_SIZE, stdin))
        {
            if (sscanf(buf, "%d%c", &id, &last_typed) == 2)
            {
                if (id && last_typed == '\n'){
                    if (id == reservation_id){
                        return;
                    }
                }
            }
        }
    }
    return;
}

/*
    Riceve dal server il codice di prenotazione previsto per quel tavolo, 
    per la data e fascia oraria corrente.
*/
void receive_reservation_ID(int socket, int *reservation_ID)
{
    int ret;
    uint32_t conv;

    ret = recv(socket, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione del codice di prenotazione\n");
        exit(1);
    }
    *reservation_ID = ntohl(conv);

    return;
}

/*
    Autenticazione del cliente per accedere ai servici del table device.
*/
bool client_authentication(char *buf, struct Socket_stream_IO *tb)
{
    int res_id = 0;

    receive_reservation_ID(tb->soc, &res_id);
    if (!res_id)
    {   
        printf("%s\n", "Il codice di prenotazione ricevuto non è valido.");
        fflush(stdout);
        return false;
    }

    printf("%s\n", "\nInserire il codice di prenotazione:\n");
    fflush(stdout);
    enter_reservation_id(buf, res_id);
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

/*
    Inoltra il comando per la ricezione del menù.
*/
void receive_menu(int sc, char *buf)
{
    int ret, length_file;
    uint32_t conv;
    FILE *f;

    conv = htonl(MENU_CMD);
    ret = send(sc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio della richiesta menu\n");
        exit(1);
    }

    length_file = 0;
    receive_file(sc, buf, &length_file);

    f = fopen("menu.txt", "w");
    if (!f)
    {
        perror("Errore nell'apertura del file menu.txt\n");
        exit(1);
    }
    fprintf(f, "%s\n", buf);
    fclose(f);
    return;
}

/*
    Fase di accensione del dispositivo: 
    invio al server del comando per la ricezione del codice
    di prenotazione previsto per il turno di lavoro corrente nel tavolo considerato.
*/
void start_device(struct Socket_stream_IO *tb, char *buf)
{
    int ret;
    uint32_t conv;

    // invio al server un segnale per ricevere il codice
    conv = htonl(TB_AUTHENTICATION_CMD);
    ret = send(tb->soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio della richiesta idb\n");
        exit(1);
    }

    if (client_authentication(buf, tb))
    {
        receive_menu(tb->soc, buf);
        printf("%s", "\n****************************** BENVENUTO ******************************\n");
        printf("%s", "Digita un comando:\n\n");
        print_cmd();
        printf("%s\n", buf);
        fflush(stdout);
    }
    return;
}

/*
    Controllo della correttezza del formato con il quale sono stati specificati i piatti
    nella comanda.
    Il menu' precedentemente ricevuto serve per controllare che non siano
    stati inseriti codici di piatti non presenti e dunque non ordinabili.
    Il vettore tot_quantity_counter serve a tenere traccia, per ogni piatto specificato e
    presente nel menu', delle quantita' complessive richieste.
*/
bool check_format(int *tot_quantity_counter, char **stream_reader)
{
    int i;
    struct Dish ordered_dish, menu_dish;
    char typed_dish[MAX_SIZE];

    FILE *f = fopen("menu.txt", "r");
    if (!f)
    {
        perror("Errore nell'apertura del file menu.txt\n");
        exit(1);
    }

    memset(&ordered_dish, 0, sizeof(struct Dish));
    memset(&menu_dish, 0, sizeof(struct Dish));
    memset(typed_dish, 0, MAX_SIZE);

    // preleva l'ID del piatto ordinato con relativa quantita'
    while (sscanf(*stream_reader, " %[^\n ] ", typed_dish) == 1)
    {
        if (sscanf(typed_dish, " %[^-]-%d ", ordered_dish.dish_ID, &ordered_dish.quantity) == 2)
        {
            rewind(f);
            i = 0;
            // controllo l'ID prelevato con ogni piatto presente nel menu'
            while (!feof(f))
            {
                if (fscanf(f, " %[^ ]%*[^\n] ", menu_dish.dish_ID))
                {
                    if (!strcmp(menu_dish.dish_ID, ordered_dish.dish_ID))
                    {
                        tot_quantity_counter[i] += ordered_dish.quantity;
                        break;
                    }
                    i++;
                }
            }
            if (i == MENU_SIZE)
                break;
        }
        else
        {
            i = MENU_SIZE;
            break;
        }
        *stream_reader += (strlen(typed_dish) + 1);
    }
    // in questo caso non e' stato trovato nel menu un piatto con quell'ID
    // ed e' sufficiente uscire dal controllo senza procedere
    if (i == MENU_SIZE)
    {
        fclose(f);
        printf("Comanda errata\n");
        fflush(stdout);
        return false;
    }

    fclose(f);
    return true;
}

/*
    Inoltra al server la richiesta di preparazione dell'ordine.
    Questa consiste nell'invio del numero complessivo di ordini effettuati da quel tavolo
    seguito dall'invio dell'ordine.
*/
void prepare_order(int soc, struct Order *ord, char *buf, int port)
{
    int ret;
    uint32_t conv;

    // invio il numero complessivo di ordini effettuato
    ord->order_counter++;
    conv = htonl(ord->order_counter);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del numero complessivo di ordini effettuati\n");
        exit(1);
    }

    send_file(soc, buf, 0);
    return;
}

void receive_order_confirmation(int soc, char *dishes_list, struct Order *ord)
{
    int ret, cmd;
    uint32_t conv;

    ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione dell'ACK per la corretta ricezione dell'ordine\n");
        exit(1);
    }
    cmd = ntohl(conv);

    if (cmd == ORDER_ACK)
    {
        printf("COMANDA RICEVUTA\n");
        fflush(stdout);
        ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nella ricezione dell'ID dell'ordine\n");
            exit(1);
        }
        ord->order_ID = ntohl(conv);
        printf("Ordine N: %d\n %s\n", ord->order_ID, dishes_list);
        fflush(stdout);
    }
    else
    {
        printf("L'invio dell'ordine non è andato a buon fine.\n");
        fflush(stdout);
    }
    return;
}

bool take_order(char *buf, struct Order *ord)
{
    int i;
    char *stream_reader = &buf[8];
    int tot_quantity_counter[MENU_SIZE];

    memset(tot_quantity_counter, 0, sizeof(tot_quantity_counter));

    if (check_format(tot_quantity_counter, &stream_reader))
    {
        for (i = 0; i < MENU_SIZE; i++)
        {
            if (tot_quantity_counter[i])
                ord->quantity[i] += tot_quantity_counter[i];
        }
        return true;
    }
    return false;
}

bool generate_recap_orders(char *buf, int *quantity)
{
    int i;
    FILE *f, *fs;
    char *stream_reader = &buf[0];
    bool outcome = false;

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
        return outcome;
    }

    rewind(fs);
    for (i = 0; i < MENU_SIZE; i++)
    {
        if (fscanf(f, " %2c%*[^\n] ", stream_reader))
        {
            if (quantity[i])
            {
                fprintf(fs, "%-3s %-4d\n", stream_reader, quantity[i]);
                outcome = true;
            }
            else
                strcpy(stream_reader, "");
        }
    }
    fclose(fs);
    fclose(f);
    return outcome;
}

void send_recap(int soc, char *buf)
{
    FILE *f;
    char *stream_reader = &buf[0];
    memset(buf, 0, MAX_SIZE);

    f = fopen("recap.txt", "r");
    if (f)
    {
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
    }
    send_file(soc, buf, 0);
    return;
}

void receive_full_order_amount(int soc, char *buf)
{
    char dish[MAX_SIZE];
    char *stream_reader = &buf[0];

    uint32_t conv;
    int ret;

    memset(dish, 0, MAX_SIZE);

    while (sscanf(stream_reader, " %[^\n] ", dish) == 1)
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
        stream_reader += (strlen(dish) + 1);
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

/*
    Controlla che la porta associata dall'utente al momento
    della chiamata al programma sia effettivamente corretta
*/
bool check_port_table(int port)
{
    if (port > TABLE_1_PORT && port < TABLE_4_PORT)
        return true;
   
    return false;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in server_address, my_address;
    struct Order ord;
    int ret, command, port;
    uint32_t conv;
    
    char buf[MAX_SIZE];
    struct Socket_stream_IO tb;

    memset(&tb, 0, sizeof(struct Socket_stream_IO));
    memset(&ord, 0, sizeof(struct Order));

    port = atoi(argv[1]);
    if (!check_port_table(port))
    {
        perror("Porta errata\n");
        exit(1);
    }

    tb.soc = create_soc_client(&server_address, &my_address, port);
    start_device(&tb, buf);
    init_stream_IO(&tb);

    while (1)
    {
        tb.read_fds = tb.master;

        ret = select(tb.max_fd + 1, &tb.read_fds, NULL, NULL, NULL);
        if (ret < 0)
        {
            perror("Errore nella select\n");
            exit(1);
        }

        memset(buf, 0, MAX_SIZE);
        command = 0;

        if (FD_ISSET(tb.soc, &tb.read_fds))
        {
            ret = recv(tb.soc, (void *)&conv, sizeof(uint32_t), 0);
            if (ret < 0)
            {
                perror("Errore nel messaggio ricevuto\n");
                exit(1);
            }
            command = ntohl(conv);

            if (command == STOP_SERVER_CMD)
            {
                cleanup_table_device(&tb);
                return 0;
            }
            else if (command == TB_AUTHENTICATION_CMD)
            {
                if (client_authentication(buf, &tb))
                {
                    receive_menu(tb.soc, buf);
                    printf("%s", "\n****************************** BENVENUTO ******************************\n");
                    printf("%s", "Digita un comando:\n\n");
                    print_cmd();
                    printf("%s\n", buf);
                    fflush(stdout);
                }
            }
            else if (command == UPDATE_STATUS_ORD_1 || command == UPDATE_STATUS_ORD_2)
            {
                receive_info_order(&ord, tb.soc);
                if (command == UPDATE_STATUS_ORD_1)
                {
                    printf("Ordine n.%d in preparazione\n", ord.order_ID);
                }
                else
                {
                    printf("Ordine n.%d in servizio\n", ord.order_ID);
                }
                fflush(stdout);
            }
        }
        else
        {
            if (fgets(buf, MAX_SIZE, stdin))
            {
                if (!strcmp(buf, "help\n"))
                    print_cmd();
                else if (!strcmp(buf, "menu\n"))
                    print_menu(buf);
                else if (!strcmp(buf, "conto\n"))
                {
                    if (generate_recap_orders(buf, ord.quantity))
                    {
                        command = END_TB_CONNECTION;
                        conv = htonl(command);
                        ret = send(tb.soc, (void *)&conv, sizeof(uint32_t), 0);
                        if (ret < 0)
                        {
                            perror("Errore nell'invio del comando\n");
                            exit(1);
                        }

                        send_recap(tb.soc, buf);
                        receive_full_order_amount(tb.soc, buf);
                        cleanup_table_device(&tb);
                        break;
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
                        conv = htonl(COMANDA_CMD);
                        ret = send(tb.soc, (void *)&conv, sizeof(uint32_t), 0);
                        if (ret < 0)
                        {
                            perror("Errore nell'invio del comando\n");
                            exit(1);
                        }
                        prepare_order(tb.soc, &ord, &buf[8], port);
                        receive_order_confirmation(tb.soc, &buf[8], &ord);
                    }
                }
            }
        }
    }
    return 0;
}
