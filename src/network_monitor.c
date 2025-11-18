#include "../include/network.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Lê estatísticas de rede do /proc/[pid]/net/dev
bool get_network_data(int pid, ResourceData *data) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/proc/%d/net/dev", pid);

    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        // Arquivo pode não existir se o processo não usa a rede, não é um erro fatal
        data->net_rx_bytes = 0;
        data->net_tx_bytes = 0;
        data->net_rx_packets = 0;
        data->net_tx_packets = 0;
        return true;
    }

    char line[512];
    long long total_rx_bytes = 0, total_tx_bytes = 0;
    long long total_rx_packets = 0, total_tx_packets = 0;

    // Pula as duas primeiras linhas (cabeçalho)
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        char iface[64];
        long long rx_bytes, tx_bytes, rx_packets, tx_packets;
        // Formato do /proc/net/dev:
        // Inter-|   Receive                                                |  Transmit
        //  face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
        sscanf(line, "%63s %lld %lld %*d %*d %*d %*d %*d %*d %lld %lld", 
               iface, &rx_bytes, &rx_packets, &tx_bytes, &tx_packets);

        // Ignora a interface de loopback
        if (strcmp(iface, "lo:") != 0) {
            total_rx_bytes += rx_bytes;
            total_tx_bytes += tx_bytes;
            total_rx_packets += rx_packets;
            total_tx_packets += tx_packets;
        }
    }

    data->net_rx_bytes = total_rx_bytes;
    data->net_tx_bytes = total_tx_bytes;
    data->net_rx_packets = total_rx_packets;
    data->net_tx_packets = total_tx_packets;

    fclose(fp);
    return true;
}
