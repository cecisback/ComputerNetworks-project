#include "common.c"

void print_cmd()
{
    printf("\nfind --> ricerca la disponibilita' per una prenotazione\n");
    printf("book --> invia una prenotazione\n");
    printf("esc --> termina il client\n");
    printf("\n");
    fflush(stdout);
    return;
}

bool check_input_cmd(char **scorri, int *choice)
{
    bool ok = false;
    char cmd[MAX_SIZE];
    memset(&cmd, 0, MAX_SIZE);

    if (sscanf(*scorri, "%100[^\n]", cmd))
    {
        if (!strcmp(cmd, "esc"))
        {
            *choice = 3;
            ok = true;
        }
    }

    if (sscanf(*scorri, "%100[^ ]", cmd))
    {
        if (!strcmp(cmd, "find"))
        {
            *choice = 1;
            ok = true;
        }
        else if (!strcmp(cmd, "book") && *choice)
        {
            *choice = 2;
            ok = true;
        }
    }
    *scorri += (strlen(cmd) + 1);

    return ok;
}

bool checkdate(int *d)
{
    bool ret = false;
    struct tm *timeinfo = current_date();

    // nell'ordine: d[0] giorno, d[1] mese, d[2] anno, d[3] ora
    if (d[2] > 22 && d[2] < 25)
    {
        if (d[1] > 0 && d[1] < 13 && d[0] > 0 && d[0] < 32)
        {
            if ((d[1] == 2 && d[0] < 29) || (d[1] != 2))
            {
                if ((d[3] > 11 && d[3] < 15) || (d[3] > 18 /*&& d[3] < 23*/))
                {
                    if (d[2] == 23)
                    {
                        if (d[1] > (timeinfo->tm_mon + 1))
                            ret = true;
                        else if (d[1] == (timeinfo->tm_mon + 1))
                        {
                            if ((d[0] > timeinfo->tm_mday) || (d[0] == timeinfo->tm_mday && d[3] > timeinfo->tm_hour))
                                ret = true;
                            else
                                ret = false;
                        }
                        else
                            ret = false;
                    }
                    else
                        ret = true;
                }
            }
        }
    }
    return ret;
}

bool take_date(char **scorri, int *date)
{
    char check[MAX_SIZE];
    int i;

    for (i = 0; i < DATE_SIZE; i++)
    {
        memset(&check, 0, MAX_SIZE);

        if ((sscanf(*scorri, "%100[^-]", check) && (i == 0 || i == 1)) || ((sscanf(*scorri, "%100[^ ]", check)) && (i == 2)) ||
            (sscanf(*scorri, "%100[^\n]", check) && (i == 3)))
        {
            if (strlen(check) == 2)
                date[i] = strtol(check, NULL, 10);
            if (!date[i])
                return false;

            *scorri += 3;
        }
    }
    return true;
}

bool check_input_opz(char **scorri, int n_opz, int *opz_choose)
{
    char check;

    if (sscanf(*scorri, "%d%c", opz_choose, &check))
    {
        if ((*opz_choose && *opz_choose <= n_opz) && (check == '\n'))
        {
            (*opz_choose)--;
            return true;
        }
    }
    return false;
}

bool check_format(char **scorri, int choice, struct booking *bk, int n_opz)
{
    char conv_to_string[MAX_SIZE];
    memset(conv_to_string, 0, MAX_SIZE);

    if (choice == 1)
    {
        memset(bk, 0, sizeof(*bk));

        if (sscanf(*scorri, "%20[^ ]%*c%d[^ ]", bk->dbk.surname, &bk->dbk.numppl) == 2)
        {
            sprintf(conv_to_string, "%d", bk->dbk.numppl);
            *scorri += (2 + strlen(conv_to_string) + strlen(bk->dbk.surname));

            if (take_date(scorri, bk->date))
            {
                if (checkdate(bk->date))
                {
                    if (bk->date[3] > 11 && bk->date[3] < 15)
                        bk->date[3] = 1;
                    else if (bk->date[3] > 19 && bk->date[3] < 21)
                        bk->date[3] = 0;

                    return true;
                }
            }
        }
    }
    else if (choice == 2)
    {
        return check_input_opz(scorri, bk->n_opz, &bk->dbk.opz_choose);
    }
    else if (choice == 3)
        return true;

    return false;
}

int print_opz(char *buf, int *choice)
{
    char *scorri = &buf[0];
    char opz[MAX_SIZE];
    int i = 0; // contatore del numero di opzione

    memset(&opz, 0, MAX_SIZE);

    while (sscanf(scorri, "%[^\n] ", opz) == 1)
    {
        printf("%c", '\n');
        printf("%d%s%s", ++i, ") ", opz);
        fflush(stdout);
        scorri += (strlen(opz) + 1);
        memset(&opz, 0, MAX_SIZE);
    }

    if (!i)
    {
        printf("Disponibilita' per la data selezionata esaurita\n");
        *choice = 0;
    }
    else
    {
        printf("%c\n", '\n');
        fflush(stdout);
    }

    return i;
}

void send_info_bk(int sc, char *buf, struct booking *bk)
{
    int ret, i;
    uint32_t conv;
    char surname[MAX_DIM_SURNAME];
    memset(&surname, 0, MAX_DIM_SURNAME);

    for (i = 0; i < DATE_SIZE; i++)
    {
        conv = htonl(bk->date[i]);
        ret = send(sc, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nell'invio della data di prenotazione\n");
            exit(1);
        }
    }

    conv = htonl(bk->dbk.numppl);
    ret = send(sc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del numero di persone specificate\n");
        exit(1);
    }

    if (sscanf(buf, "%*5c%[^ ]", surname) == 1)
    {
        ret = send(sc, (void *)surname, MAX_DIM_SURNAME, 0);
        if (ret < 0)
        {
            perror("Errore nell'invio del cognome\n");
            exit(1);
        }
    }
    return;
}

void send_opzchoose(int sc, int choice)
{
    int ret;
    uint32_t conv;

    conv = htonl(choice);
    ret = send(sc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio dell'opzione scelta\n");
        exit(1);
    }
    return;
}

void rqt_opz(int sc, char *buf, int *choice, struct booking *bk)
{
    int len = 0;
    send_info_bk(sc, buf, bk);
    rcv_file(sc, buf, &len);
    bk->n_opz = print_opz(buf, choice);
    return;
}

bool rcv_ACK_bk(int sc, char *buf, int *id_b)
{
    int quanti, ret, conv;
    quanti = ACK_FOR_BOOK;

    rcv_file(sc, buf, &quanti);
    printf("%s\n", buf);
    fflush(stdout);

    quanti = 0;
    rcv_file(sc, buf, &quanti);
    printf("%s", buf);
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

int main(int argc, char *argv[])
{
    struct sockaddr_in server_address, my_address;
    struct booking bk;
    int ret, sc, choice;
    char *scorri = NULL;
    char buf[MAX_SIZE];
    uint32_t conv;

    if (argc == 2)
    {
        if (!check_port(atoi(argv[1]), 1))
        {
            perror("Porta errata\n");
            exit(1);
        }
    }

    sc = create_sc_client(&server_address, &my_address, 7000);
    print_cmd();
    choice = 0;

    while (1)
    {
        memset(buf, 0, MAX_SIZE);
        scorri = &buf[0];

        if (fgets(buf, MAX_SIZE, stdin))
        {
            if (check_input_cmd(&scorri, &choice))
            {
                if (check_format(&scorri, choice, &bk, bk.dbk.opz_choose))
                {
                    conv = htonl(choice);
                    ret = send(sc, (void *)&conv, sizeof(uint32_t), 0);
                    if (ret < 0)
                    {
                        perror("Errore nell'invio del comando\n");
                        exit(1);
                    }

                    switch (choice)
                    {
                    case 1:
                        rqt_opz(sc, buf, &choice, &bk);
                        break;
                    case 2:
                        send_opzchoose(sc, bk.dbk.opz_choose);
                        if (rcv_ACK_bk(sc, buf, &bk.dbk.id_b))
                            choice = 0;
                        break;
                    case 3:
                        close(sc);
                        break;
                    }
                    if (choice == 3)
                        break;
                }
            }
        }
    }
    return 0;
}
