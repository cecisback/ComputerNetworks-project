#include "common.c"

void print_cmd()
{
    printf("%s", "take -. accetta una comanda\n");
    printf("%s", "show -. mostra le comande accettate (in preparazione)\n");
    printf("%s", "set -. imposta lo stato della comanda\n");
    printf("%s", "\n");
    fflush(stdout);
    return;
}

void rcv_ord_pnd(int soc)
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

void show_acp(char *buf, struct order *ord)
{
    int ok = 0;
    FILE *f = fopen("accepted.txt", "r");
    if (!f)
    {
        printf("Non ci sono comande accettate\n");
        fflush(stdout);
        return;
    }

    memset(buf, 0, MAX_SIZE);

    rewind(f);
    while (!feof(f))
    {
        if (fscanf(f, " %[^\n] ", buf))
        {
            if (sscanf(buf, "ID:%*3c com%*d T%*d STATUS = %c\n", &ord->sts))
            {
                if (ord->sts == '1')
                {
                    strcpy(&buf[16], "");
                    ok = 1;
                }
                else
                    ok = 0;
            }

            if (ok == 1)
            {
                printf("%s\n", buf);
                fflush(stdout);
            }
        }
    }
    fclose(f);
    return;
}

void save_kd_acp(char *buf)
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
    fprintf(f, "%s STATUS = 1\n", ord);
    fprintf(f, "%s", &buf[16]);
    fclose(f);
    return;
}

void set_ready(char *n_file, struct order *ord)
{
    fpos_t pos;
    struct order ord_found;
    FILE *f = fopen(n_file, "r+");
    if (!f)
    {
        printf("Errore nell'apertura del file\n");
        fflush(stdout);
        return;
    }

    memset(&ord_found, 0, sizeof(struct order));
    pos.__pos = 0;

    rewind(f);
    while (!feof(f))
    {
        if (fscanf(f, "ID:%d com%d T%d STATUS = %*c\n", &ord_found.id_ord, &ord_found.n_ord, &ord_found.id_tb) == 3)
        {
            if (ord_found.id_tb == ord->id_tb && ord_found.n_ord == ord->n_ord)
            {
                fgetpos(f, &pos);
                ord->id_ord = ord_found.id_ord;
                ord->sts = '2';
                pos.__pos -= 2;
                fsetpos(f, &pos);
                fputc(ord->sts, f);
                break;
            }
        }
        else
            fscanf(f, " %*[^\n] ");
    }
    fclose(f);

    if (!ord->id_ord)
    {
        printf("Impossibile aggiornare la comanda\n");
        fflush(stdout);
    }
    return;
}

int main(int argc, char *argv[])
{
    int i;
    struct sockaddr_in server_address, my_address;
    struct select_elem kd;
    struct order ord;
    uint32_t conv;
    int ret, choice;
    char buf[MAX_SIZE];

    memset(&kd, 0, sizeof(struct select_elem));

    if (argc == 2)
    {
        port = atoi(argv[1]);
        if (!check_port(port, 2))
            ;
        {
            perror("Porta errata\n");
            exit(1);
        }
    }

    kd.soc = create_sc_client(&server_address, &my_address, 6001);
    init_pre_select(&kd);
    print_cmd();

    // invio segnale per ricevere gli ordini pendenti
    choice = -1;
    conv = htonl(choice);
    ret = send(kd.soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        printf("Errore nell'invio del segnale per la ricezione degli ordini pendenti\n");
        exit(1);
    }
    rcv_ord_pnd(kd.soc);

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
            memset(&ord, 0, sizeof(struct order));
            choice = 0;

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

                    choice = ntohl(conv);
                    if (choice == -2)
                    {
                        printf("Chiusura del dispositivo in corso...\n");
                        fflush(stdout);
                        remove("accepted.txt");
                        close(kd.soc);
                        FD_ZERO(&kd.master);
                        FD_ZERO(&kd.read_fds);
                        return 0;
                    }
                    else if (choice == -1)
                    {
                        rcv_ord_pnd(kd.soc);
                    }
                    else if (choice == 1)
                    {
                        ret = 0;
                        rcv_file(kd.soc, buf, &ret);
                        printf("%s\n", buf);
                        fflush(stdout);
                        save_kd_acp(buf);
                    }
                }
                else if (i == STDIN_FILENO)
                {
                    if (fgets(buf, MAX_SIZE, stdin))
                    {
                        if (!strcmp(buf, "take\n"))
                        {
                            choice = 1;
                            conv = htonl(choice);
                            ret = send(kd.soc, (void *)&conv, sizeof(uint32_t), 0);
                            if (ret < 0)
                            {
                                perror("Errore nell'invio del comando digitato\n");
                                exit(1);
                            }
                        }
                        else if (!strcmp(buf, "show\n"))
                        {
                            show_acp(buf, &ord);
                        }
                        else if (sscanf(buf, "set com%d-T%d\n", &ord.n_ord, &ord.id_tb) == 2)
                        {
                            ord.sts = '1';
                            update_sts("accepted.txt", '2', &ord);
                            if (ord.id_ord)
                            {
                                choice = 3;
                                conv = htonl(choice);
                                ret = send(kd.soc, (void *)&conv, sizeof(uint32_t), 0);
                                if (ret < 0)
                                {
                                    perror("Errore nell'invio del comando digitato\n");
                                    exit(1);
                                }
                                send_info_ord(&ord, kd.soc);
                            }
                            else
                            {
                                printf("%s\n", "Non e' possibile portare in stato in servizio la comanda selezionata\n");
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
