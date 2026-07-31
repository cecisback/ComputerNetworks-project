#include "client_side/headers/header_client.h"

void print_command_options()
{
    printf("\nfind --> ricerca la disponibilita' per una prenotazione\n");
    printf("book --> invia una prenotazione\n");
    printf("esc --> termina il client\n");
    printf("\n");
    fflush(stdout);
    return;
}

/*
    Controlla che il comando digitato dall'utente corrisponda ad uno dei comandi previsti dal
    sistema e visibili nella funzione 'print_command_options()'.
    In caso positivo, inoltra al server l'identificativo associato al servizio richiesto.
*/
bool input_scan(char **stream_reader, int *command)
{
    bool valid = false;

    char buf[MAX_SIZE];
    memset(&buf, 0, MAX_SIZE);

    if (sscanf(*stream_reader, "%100[^\n]", buf))
    {
        if (!strcmp(buf, "esc"))
        {
            *command = ESC_CMD;
            valid = true;
        }
    }

    if (sscanf(*stream_reader, "%100[^ ]", buf))
    {
        if (!strcmp(buf, "find"))
        {
            *command = FIND_CMD;
            valid = true;
        }
        else if (!strcmp(buf, "book") && *command)
        {
            *command = BOOK_CMD;
            valid = true;
        }
    }
    *stream_reader += (strlen(buf) + 1);

    return valid;
}

bool check_lunch_shift(int hour){
    if (hour > 11 && hour < 16){
        return true;
    }
    return false;
}

bool check_dinner_shift(int hour){
    if (hour > 18 && hour < 22){
        return true;
    }
    return false;
}

bool check_valid_month(int month){
    if (month > 0 && month < 13){
        return true;
    }
    return false;
}

bool check_valid_year(int typed_year, int current_year){
    if (typed_year >= current_year && typed_year < current_year+2){
        return true;
    }
    return false;
}

bool check_valid_day(int day, int month){
    if (day > 0 && day < 32){
        if (month == 2 && day > 28){
            return false;
        }
        return true;
    }
    return false;
}

bool check_current_year(int current_month, int current_day, int current_hour, int selected_month, int selected_day, int selected_hour){
    int selected_shift = check_lunch_shift(selected_hour)? 1 :(check_dinner_shift(selected_hour)? 0 : -1);
    int current_shift = check_lunch_shift(current_hour)? 1 :(check_dinner_shift(current_hour)? 0 : -1);

    if (selected_shift == -1 || current_shift == -1){
        return false;
    }
    
    if (selected_month == current_month){
        if (selected_day == current_day){
            return (selected_shift <= current_shift)? true : false;
        }else{
            if (selected_day > current_day){
                return true;
            }
        }
    }else{
        if (selected_month > current_month){
            return true;
        }
    }
    return false;
}

/*
    Accerta la validità della data specificata dall'utente al momento della prenotazione.
    Il confronto viene fatto tenendo conto della data odierna e di alcuni vincoli relativi al formato 
    della data recuperata dalla prenotazione e di quella estratta dalla struttura tm (definita in header.h).
    Nello specifico vegono eseguiti due tipi di controllo.
    Il primo, più generico, verifica che:
    - la prenotazione è relativa all'anno corrente o al più all'anno successivo;
    - il valore indicato come giorno e mese nella data prenotata rientra nei range [1,31] e [1,12] rispettivamente;
    - l'orario della prenotazione è compreso tra [12,15] o [19,21], range di orari corrispondenti ai turni lavorativi di pranzo e cena;

    Il secondo controllo, più specifico, viene effettuato solo nel caso in cui la prenotazione è relativa all'anno e mese correnti.
    L'obiettivo principale è garantire che la prenotazione richiesta dall'utente sia coerente con la data
    attuale.
*/
bool check_date(int *date)
{
    struct tm *timeinfo = current_date();

    int current_year = timeinfo->tm_year - 2000;
    int current_month = timeinfo->tm_mon + 1;
    int current_day = timeinfo->tm_mday;
    int current_hour = timeinfo->tm_hour;

    bool valid = false;

    if (check_valid_year(date[2], current_year))
    {
        if (check_valid_month(date[1]) && check_valid_day(date[0], date[1]))
        {
            if (check_lunch_shift(date[3]) || check_dinner_shift(date[3]))
            {
                if (date[2] == current_year)
                {
                    valid = check_current_year(current_month, current_day, current_hour, date[1], date[0], date[3]);
                }else{
                    valid = true;
                }
            }
        }
    }

    return valid;
}

/*
    Per estrarre la data della prenotazione dall'input dell'utente.
    Nell'array date di lunghezza DATE_SIZE (4) sono contenuti rispettivamente, nell'ordine riportato: 
    giorno, mese, anno e orario.
    Il formato previsto per la data di prenotazione è: DD-MM-YY HH\n.
*/
bool retrieve_date(char **stream_reader, int *date)
{
    char buf[MAX_SIZE];
    int i;

    for (i = 0; i < DATE_SIZE; i++)
    {
        memset(&buf, 0, MAX_SIZE);

        if ((sscanf(*stream_reader, "%100[^-]", buf) && (i == 0 || i == 1)) || 
            ((sscanf(*stream_reader, "%100[^ ]", buf)) && (i == 2)) ||
            (sscanf(*stream_reader, "%100[^\n]", buf) && (i == 3)))
        {
            if (strlen(buf) == 2)
                date[i] = strtol(buf, NULL, 10);

            if (!date[i])
                return false;

            *stream_reader += 3;
        }
    }
    return true;
}

/*
    Controlla che il tavolo selezionato dall'utente dopo la proposta effettuata dal sistema
    sulla base delle disponibilità per la data indicata sia valida.
    Nello specifico, controlla che l'utente abbia digitato esattamente un numero e che 
    questo corrisponda all'indice di una proposta valida, precedentemente generata e visualizzata
    su schermo.
    In caso positivo, restituisce true e aggiorna il contenuto del paramentro in input selected_tb
    con l'indice relativo al tavolo selezionato, così da estrarlo dalla struttura dati contenente tutte
    le disponibilità recuperate.
*/
bool check_selected_table(char **stream_reader, int n_available_tables, int *selected_tb)
{
    char last;
    bool end_stream = false;

    if (sscanf(*stream_reader, "%d%c", selected_tb, &last))
    {
        if (last == '\n'){
            end_stream = true;
        }

        if ((*selected_tb && *selected_tb <= n_available_tables) && (end_stream == true))
        {
            (*selected_tb)--;
            return true;
        }
    }
    return false;
}

/*
    Per la creazione di una nuova richiesta di prenotazione da parte dell'utente.
    Se l'obiettivo è recuperare le disponibilità per la data selezionata esegue le seguenti operazioni:
    - inizializza a zero i campi della nuova struttura dati Reservation_request;
    - Legge in input i valori specificati per la prenotazione: quali cognome, numero di clienti;
    - Controlla che la data indicata sia valida (nel formato e nella coerenza rispetto alla data corrente);

    Altrimenti, se l'obiettivo è la prenotazione del tavolo, e quindi la conferma della selezione 
    dell'utente dopo la proposta delle disponibilità, controlla l'input digitato e ricava l'indice corrispondente
    alla proposta scelta per indicizzare l'array available_table_IDs nella struttura dati Reservation_request.
*/
bool check_reservation_request_format(char **stream_reader, int command, struct Reservation_request *request)
{
    char buf[MAX_SIZE];
    memset(buf, 0, MAX_SIZE);

    if (command == FIND_CMD)
    {
        memset(request, 0, sizeof(*request));

        if (sscanf(*stream_reader, "%20[^ ]%*c%d[^ ]", request->reservation_info.surname, &request->reservation_info.num_guests) == 2)
        {
            request->reservation_info.surname[strlen(request->reservation_info.surname)-1] = '\n';
            sprintf(buf, "%d", request->reservation_info.num_guests);
            *stream_reader += (2 + strlen(buf) + strlen(request->reservation_info.surname));

            if (retrieve_date(stream_reader, request->date))
            {
                if (check_date(request->date))
                {   
                    request->date[3] = check_dinner_shift(request->date[3])? 1 : 0;
                    return true;
                }
            }
        }
    }
    else if (command == BOOK_CMD)
    {
        return check_selected_table(stream_reader, request->tot_available_tables, &request->reservation_info.selected_table);
    }

    return false;
}

/*
    Stampa a video le disponibilità di tavoli trovati in sala per la richiesta di prenotazione 
    inoltrata al server e restituisce il numero totale di tavoli disponibili.
*/
int print_available_tables(char *available_tables_list)
{
    char *stream_reader = &available_tables_list[0];
    int num_available_tables = 0;

    char buf[MAX_SIZE];
    memset(&buf, 0, MAX_SIZE);

    while (sscanf(stream_reader, "%[^\n] ", buf) == 1)
    {
        printf("\n%d%2s%s", ++num_available_tables, ") ", buf);
        fflush(stdout);
        stream_reader += (strlen(buf) + 1);
        memset(&buf, 0, MAX_SIZE);
    }

    if (num_available_tables == 0)
    {
        printf("Disponibilita' per la data selezionata esaurita\n");
    }
    else
    {
        printf("%2s", "\n\n");
    }

    fflush(stdout);
    return num_available_tables;
}

/*
    Per inoltrare al server le informazioni processate relative alla nuova richiesta di prenotazione.
    Nello specifico, invia la data (giorno, mese, anno e turno), il numero di persone che richiedono il 
    posto a sedere e il cognome specificato per riservare il tavolo.
    Le informazioni saranno poi gestite lato server per recuperare le disponibilità dei tavoli in sala,
    che saranno selezionati in base al numero di persone richieste e le altre prenotazioni per lo stesso
    turno di lavoro nella stessa data.
*/
void send_reservation_request(int sc, char *buf, struct Reservation_request *request)
{
    int ret, i;
    uint32_t conv;

    char surname[SURNAME_SIZE];
    memset(&surname, 0, SURNAME_SIZE);

    for (i = 0; i < DATE_SIZE; i++)
    {
        conv = htonl(request->date[i]);
        ret = send(sc, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nell'invio della data di prenotazione\n");
            exit(1);
        }
    }

    conv = htonl(request->reservation_info.num_guests);
    ret = send(sc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del numero di persone specificate\n");
        exit(1);
    }

    if (sscanf(buf, "%*5c%[^ ]", surname) == 1)
    {
        ret = send(sc, (void *)surname, SURNAME_SIZE, 0);
        if (ret < 0)
        {
            perror("Errore nell'invio del cognome\n");
            exit(1);
        }
    }
    return;
}

void send_selected_table(int socket, int choice)
{
    int ret;
    uint32_t conv;

    conv = htonl(choice);
    ret = send(socket, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del tavolo selezionato\n");
        exit(1);
    }
    return;
}

/*
    Per inoltrare al server i dettagli relativi alla nuova richiesta di prenotazione, attendere la ricezione
    delle disponibilità in sala per la data e il numero di persone specificati e stampare a video
    le informazioni relative a ciascun tavolo disponibile.f
*/
void handle_reservation_request(int socket, char *buf, struct Reservation_request *req)
{
    int length_file = 0;
    send_reservation_request(socket, buf, req);
    receive_file(socket, buf, &length_file);
    req->tot_available_tables = print_available_tables(buf);
    printf("%s%d\n","La richiesta di prenotazione e' stata correttamente processata. Numero complessivo di tavoli dispobili trovati per la data selezionata: ", req->tot_available_tables);
    return;
}

bool receive_booking_confirmation(int sc, char *buf, int *id_b)
{
    int ret, ack, length_file;
    uint32_t conv;

    ret = recv(sc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione della conferma di prenotazione\n");
        exit(1);
    }
    ack = ntohl(conv);

    if (ack != RESERVATION_ACK){
        return false;
    }

    length_file = 0;
    receive_file(sc, buf, &length_file);
    printf("%s\n", buf);
    fflush(stdout);

    // riceve id della prenotazione
    ret = recv(sc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione dell'id prenotazione\n");
        exit(1);
    }
    *id_b = ntohl(conv);
    printf("%d\n\n", *id_b);
    fflush(stdout);

    return true;
}

/*
    Controlla che la porta associata dall'utente al momento
    della chiamata al programma sia effettivamente corretta
*/
bool check_port_client(int port)
{
    if (port == CLIENT_PORT)
        return true;
    return false;
}


int main(int argc, char *argv[])
{
    struct sockaddr_in server_address, my_address;
    struct Reservation_request req;
    int ret, socket, command;
    char *stream_reader = NULL;
    char buf[MAX_SIZE];
    uint32_t conv;

    if (argc == 2)
    {
        if (!check_port_client(atoi(argv[1])))
        {
            perror("Porta errata\n");
            exit(1);
        }
    }

    socket = create_soc_client(&server_address, &my_address, CLIENT_PORT);
    print_command_options();
    command = 0;

    while (1)
    {
        memset(buf, 0, MAX_SIZE);
        stream_reader = &buf[0];

        if (fgets(buf, MAX_SIZE, stdin))
        {
            if (input_scan(&stream_reader, &command))
            {
                if (check_reservation_request_format(&stream_reader, command, &req))
                {
                    // Inoltro del comando digitato al server
                    conv = htonl(command);
                    ret = send(socket, (void *)&conv, sizeof(uint32_t), 0);
                    if (ret < 0)
                    {
                        perror("Errore nell'invio del comando\n");
                        exit(1);
                    }

                    switch(command){
                        case FIND_CMD:
                            handle_reservation_request(socket,buf,&req);
                            break;
                        case BOOK_CMD:
                            send_selected_table(socket, req.reservation_info.selected_table);
                            if (receive_booking_confirmation(socket,buf, &req.reservation_info.id)){
                                command = 0;
                            }
                            break;
                        case ESC_CMD:
                            close(socket);
                            break;
                    }

                    if (command == ESC_CMD){
                        break;
                    }

                } else {
                    printf("%s\n", "La richiesta di prenotazione non è stata correttamente processata.");
                }
            }else{
                printf("%s\n", "Il comando digitato in input non è valido.");
            }
        }
    }
    return 0;
}
