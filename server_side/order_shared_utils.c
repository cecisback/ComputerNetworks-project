void update_status_order(char *filename, char new_status, struct Order *ord)
{
    struct Order order_found;
    fpos_t pos;

    FILE *f = fopen(filename, "r+");
    if (!f)
    {
        printf("Errore nell'apertura del file\n");
        fflush(stdout);
        return;
    }

    memset(&order_found, 0, sizeof(struct Order));
    pos.__pos = 0;

    rewind(f);
    while (!feof(f))
    {
        if (fscanf(f, "ID:%d com%d T%d STATUS = %d\n", &order_found.order_ID, &order_found.order_counter, &order_found.table_ID, &order_found.status) == 4)
        {
            if ((((order_found.table_ID == ord->table_ID && order_found.order_counter == ord->order_counter) ||
                order_found.order_ID == ord->order_ID)) &&
                (order_found.status == ord->status))
            {
                if (!ord->order_ID)
                    ord->order_ID = order_found.order_ID;
                ord->table_ID = order_found.table_ID;
                fgetpos(f, &pos);
                pos.__pos -= 2;
                fsetpos(f, &pos);
                fputc(new_status, f);

                if (new_status == '1')
                {
                    ord->status = 1;
                    printf("Comanda n.%d in preparazione\n", ord->order_ID);
                }
                else
                {
                    ord->status = 2;
                    printf("Comanda n.%d in servizio\n", ord->order_ID);
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

void send_info_order(struct Order *ord, int soc)
{
    uint32_t conv;
    int ret;

    conv = htonl(ord->order_ID);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio del codice associato all'ordine\n");
        exit(1);
    }

    conv = htonl(ord->order_counter);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0){
        perror("Errore nell'invio del numero di ordine effettuato\n");
        exit(1);
    }

    conv = htonl(ord->table_ID);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0){
        perror("Errore nell'invio dell'ID del tavolo dal quale è stato effettuato l'ordine\n");
        exit(1);
    }

    conv = htonl(ord->status);
    ret = send(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nell'invio dello stato associato all'ordine\n");
        exit(1);
    }
    return;
}