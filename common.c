#include "header.h"
#include <fstream>

/*
    Gestisce il caso di chiusura non prevista
*/
void handler(int sig)
{
    printf("\nChiusura del dispositivo non permessa con queste modalita'\n");
    return;
}

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
    Inizializzazione dell'indirizzo del server
*/
void init_server_addr(struct sockaddr_in *server_addr)
{
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(4242);
    inet_pton(AF_INET, "10.0.2.100", &server_addr->sin_addr);
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

int create_sc_client(struct sockaddr_in *s_addr, struct sockaddr_in *c_addr, int port)
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

void init_pre_select(struct select_elem *elem)
{
    FD_ZERO(&elem->master);
    FD_ZERO(&elem->read_fds);
    FD_SET(elem->soc, &elem->master);
    FD_SET(STDIN_FILENO, &elem->master);
    elem->max_fd = elem->soc;
    return;
}

/*
    Controlla che la porta associata dall'utente al momento
    della chiamata al programma sia effettivamente corretta
*/
bool check_port(int port, int service)
{
    bool ret = false;

    if ((port > 5000 && port < 5004) && service == 2)
        ret = true;
    else if ((port == 6001 || port == 6002) && service == 3)
        ret = true;
    else if (port == 7000 && service == 1)
        ret = true;
    else if (port == 4242 && service == 0)
        ret = true;

    return ret;
}

/*
    Controlla che tutti gli ordini siano stati serviti.
*/
bool check_sts_ord()
{
    int sts;
    FILE *f = fopen("ord_oftheday.txt", "r");
    if (!f)
    {
        perror("Errore nell'apertura del file\n");
        exit(1);
    }
    rewind(f);
    while (!feof(f))
    {
        if (fscanf(f, "%*25c%d", &sts))
        {
            if (sts != 2)
            {
                fclose(f);
                return false;
            }
        }
    }
    fclose(f);
    return true;
}

void send_file(int s, char *snd_buf, int tot)
{
    int letti, ret, quanti;
    uint32_t conv;
    letti = 0;

    if (!tot)
    {
        quanti = strlen(snd_buf);
        conv = htonl(quanti);
        ret = send(s, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nell' invio della dimensione del file!\n");
            exit(1);
        }
    }
    else
        quanti = tot;

    while (letti < quanti)
    {
        ret = send(s, (void *)&snd_buf[letti], quanti - letti, 0);
        if (ret < 0)
        {
            perror("Errore nella trasmissione del file!\n");
            exit(1);
        }
        letti += ret;
    }
    return;
}

void rcv_file(int socket, char *rcv_buf, int *quanti)
{
    int count, ret;
    uint32_t conv;
    count = 0;
    memset(rcv_buf, 0, MAX_SIZE);

    // Se non sa quanti byte devono essere ricevuti si calcolano e si
    // restituiscono al chiamante
    if (!*quanti)
    {
        ret = recv(socket, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nella ricezione della dimensione del file!\n");
            exit(1);
        }
        *quanti = ntohl(conv);
    }

    while (count < *quanti)
    {
        ret = recv(socket, (void *)&rcv_buf[count], *quanti - count, 0);
        if (ret < 0)
        {
            perror("Errore nella ricezione del file!\n");
            exit(1);
        }
        count += ret;
    }
    return;
}

/*
    Stampa a video lo stato in formato stringa.
*/
void show_sts_ord(char sts)
{
    switch (sts)
    {
    case '0':
        printf("<in attesa>\n");
        break;
    case '1':
        printf("<in preparazione>\n");
        break;
    case '2':
        printf("<in servizio>\n");
        break;
    }
    fflush(stdout);
    return;
}

/*
    A seconda del parametro type (0:id prenotazione, 1:idx tavolo prenotato)
    salva le informazioni necessarie per l'autenticazione del client
    o valutazione della disponibilita' tavoli in sala
*/
void save_info_found(int *info, int codfound, int *j, int type)
{
    FILE *f;
    int cod, id_tb;

    f = fopen("bk_detail.txt", "r");
    if (!f)
    {
        perror("Errore nell'apertura del file bk_detail.txt\n");
        exit(1);
    }

    while (fscanf(f, "%*5c%5d%*8c%d", &cod, &id_tb) == 2)
    {
        if (cod == codfound)
        {
            if (type == 0)
                info[id_tb] = codfound;
            else
            {
                info[(*j)++] = id_tb;
            }
            break;
        }
        else
            fscanf(f, " %*[^\n] ");
    }
    fclose(f);
    return;
}

/*
    Data la data selezionata, salva l'id del tavolo prenotato (se type = 1) per poter valutare
    le opzioni da fornire all'utente o il codice di prenotazione associato
    a ciascun tavolo (type 0).
*/
void find_booked(int *date, int *sts_tb, int type)
{
    FILE *f;
    int datefound[DATE_SIZE];
    int i, codfound, j;
    j = 0;

    f = fopen("bk_oftheday.txt", "r");
    if (!f)
        return;

    rewind(f);
    while (!feof(f))
    {
        if (fscanf(f, " %*7c%d%*c%d%*c%d%*c%d%*6c%d ", &datefound[0], &datefound[1], &datefound[2], &datefound[3], &codfound) == 5)
        {
            for (i = 0; i < DATE_SIZE; i++)
            {
                if (datefound[i] != date[i])
                    break;
            }
            /*se ha trovato almeno una prenotazione nella lista con stessa data
            e stessa fascia oraria ne legge l'identificatore*/
            if (i == DATE_SIZE)
                save_info_found(sts_tb, codfound, &j, type);
        }
        else
            break;
    }
    fclose(f);

    return;
}

/*Salva l'ordinazione ricevuta*/
void save_ord(char *buf, struct order *ord)
{
    FILE *f = fopen("ord_oftheday.txt", "a+");
    if (!f)
    {
        perror("Errore nell'apertura del file ord.txt\n");
        exit(1);
    }
    ord->sts = '0';
    fprintf(f, "ID:%-3d com%-2d T%-2d STATUS = %c\n%s", ord->id_ord, ord->n_ord, idt_tot[ord->id_tb], ord->sts, buf);
    fclose(f);
    printf("ID:%-3d com%-2d T%-2d ", ord->id_ord, ord->n_ord, idt_tot[ord->id_tb]);
    show_sts_ord(ord->sts);
    printf("%s", buf);
    fflush(stdout);
    return;
}

void update_sts(char *n_file, char new_sts, struct order *ord)
{
    fpos_t pos;
    struct order ord_found;
    FILE *f = fopen(n_file, "r+");
    if (!f)
    {
        perror("Errore nell'apertura del file\n");
        exit(1);
    }

    memset(&ord_found, 0, sizeof(struct order));
    pos.__pos = 0;

    rewind(f);
    while (!feof(f))
    {
        if (fscanf(f, "ID:%d com%d T%d STATUS = %c\n", &ord_found.id_ord, &ord_found.n_ord, &ord_found.id_tb, &ord_found.sts) == 4)
        {
            if (((ord_found.id_ord == ord->id_ord) || (ord_found.id_tb == ord->id_tb && ord_found.n_ord == ord->n_ord)) && ord_found.sts == ord->sts)
            {
                if (!ord->id_ord)
                    ord->id_ord = ord_found.id_ord;
                ord->id_tb = ord_found.id_tb;
                fgetpos(f, &pos);
                pos.__pos -= 2;
                fsetpos(f, &pos);
                fputc(new_sts, f);

                if (new_sts == '1')
                {
                    printf("Comanda n.%d in preparazione\n", ord->id_ord);
                }
                else
                {
                    printf("Comanda n.%d in servizio\n", ord->id_ord);
                }
                fflush(stdout);
                break;
            }
        }
        else
            fscanf(f, " %*[^\n] ");
    }
    fclose(f);
    return;
}

void find_oldest_pnd(char *buf, struct order *ord)
{
    fpos_t pos;
    char *scorri = &buf[15];
    FILE *f = fopen("ord_oftheday.txt", "r+");
    if (!f)
    {
        printf("Non sono presenti ordinazioni\n");
        fflush(stdout);
        return;
    }

    memset(ord, 0, sizeof(struct order));
    rewind(f);

    while (!feof(f))
    {
        memset(buf, 0, MAX_SIZE);

        if (fscanf(f, " %[^\n] ", buf))
        {
            if (!strncmp(buf, "ID:", 3))
            {
                sscanf(buf, "ID:%3d com%2d T%2d STATUS = %c\n", &ord->id_ord, &ord->n_ord, &ord->id_tb, &ord->sts);
                if (ord->sts == '0')
                {
                    fgetpos(f, &pos);
                    pos.__pos -= 2;
                    fsetpos(f, &pos);
                    ord->sts = '1';
                    fputc(ord->sts, f);
                    break;
                }
            }
        }
    }

    if (ord->sts == '1')
    {
        strcpy(scorri, "");
        strcat(scorri++, "\n");
        while (fscanf(f, " %[^\n] ", scorri) == 1)
        {
            if (strncmp(scorri, "ID:", 3))
            {
                strcat(scorri, "\n");
                scorri += strlen(scorri);
            }
            else
            {
                strcpy(scorri, "");
                break;
            }
        }

        printf("Ordine %d in preparazione\n", ord->id_ord);
        fflush(stdout);
    }
    return;
}

void rcv_info_ord(struct order *ord, int soc)
{
    uint32_t conv;
    int ret;

    // ricevo il codice associato alla comanda
    ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione dell'id dell' ordine\n");
        exit(1);
    }
    ord->id_ord = ntohl(conv);
    /*
        // ricevo il numero di ordine associato alla comanda
        ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nella ricezione del numero di ordine\n");
            exit(1);
        }
        ord->n_ord = ntohl(conv);

        // ricevo il numero di ordine associato alla comanda
        ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nella ricezione del numero di tavolo associato alla comanda\n");
            exit(1);
        }
        ord->id_tb = ntohl(conv);
    */
    // ricevo lo stato associato alla comanda
    ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione dello stato della comanda\n");
        exit(1);
    }
    ord->sts = ntohl(conv);
    return;
}

void send_info_ord(struct order *ord, int soc)
{
    uint32_t conv;
    int ret;

    // invio il codice associato alla comanda
    conv = htonl(ord->id_ord);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del codice associato alla comanda\n");
        exit(1);
    }

        // invio il numero di ordine associato alla comanda
        conv = htonl(ord->n_ord);
        ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nell'invio del numero di ordine effettuato\n");
            exit(1);
        }

        // invio il numero di tavolo associato alla comanda
        conv = htonl(ord->id_tb);
        ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nell'invio del numero di ordine effettuato\n");
            exit(1);
        }

    // invio stato associato alla comanda
    conv = htonl(ord->sts);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio dello stato associato alla comanda\n");
        exit(1);
    }
    return;
}

void show_ord(int id_tb, char sts, char *buf)
{
    bool ok = false;
    struct order ord;
    FILE *f = fopen("ord_oftheday.txt", "r");
    if (!f)
    {
        printf("Non ci sono prenotazioni per questo tavolo\n");
        return;
    }

    memset(&ord, 0, sizeof(struct order));

    rewind(f);
    while (!feof(f))
    {
        memset(buf, 0, MAX_SIZE);

        if (fscanf(f, " %[^\n] ", buf))
        {
            if (sscanf(buf, "ID:%*3c com%*d T%d STATUS = %c\n", &ord.id_tb, &ord.sts) == 2)
            {
                if (ord.sts == sts)
                {
                    strcpy(&buf[16], "");
                    ok = true;
                }
                else if (id_tb && ord.id_tb == id_tb)
                {
                    strcpy(&buf[12], "");
                    show_sts_ord(ord.sts);
                    ok = true;
                }
                else
                    ok = false;
            }

            if (ok == true)
            {
                printf("%s\n", buf);
                fflush(stdout);
            }
        }
    }

    fclose(f);
    return;
}