void receive_info_order(struct Order *ord, int soc)
{
    uint32_t conv;
    int ret;

    ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione dell'ID dell'ordine\n");
        exit(1);
    }
    ord->order_ID = ntohl(conv);

    ret = recv(soc, (void *)&conv, sizeof(uint32_t), 0);
    if (ret < 0)
    {
        perror("Errore nella ricezione dello stato dell'ordine\n");
        exit(1);
    }
    ord->status = ntohl(conv);
    return;
}