#include "header.h"

void send_file(int s, char *buf, int tot)
{
    int letti, ret, num_byte;
    uint32_t conv;
    letti = 0;

    if (!tot)
    {
        num_byte = strlen(buf);
        conv = htonl(num_byte);
        ret = send(s, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nell' invio della dimensione del file!\n");
            exit(1);
        }
    }
    else
        num_byte = tot;

    while (letti < num_byte)
    {
        ret = send(s, (void *)&buf[letti], num_byte - letti, 0);
        if (ret < 0)
        {
            perror("Errore nella trasmissione del file!\n");
            exit(1);
        }
        letti += ret;
    }
    return;
}

void receive_file(int socket, char *buf, int *num_byte)
{
    int count, ret;
    uint32_t conv;
    count = 0;

    memset(buf, 0, MAX_SIZE);

    if (!*num_byte)
    {
        ret = recv(socket, (void *)&conv, sizeof(uint32_t), 0);
        if (ret < 0)
        {
            perror("Errore nella ricezione della dimensione del file!\n");
            exit(1);
        }
        *num_byte = ntohl(conv);
    }

    while (count < *num_byte)
    {
        ret = recv(socket, (void *)&buf[count], *num_byte - count, 0);
        if (ret < 0)
        {
            perror("Errore nella ricezione del file!\n");
            exit(1);
        }
        count += ret;
    }
    return;
}