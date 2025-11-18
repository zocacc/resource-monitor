#include "../include/utils.h"
#include <stdio.h>

void export_to_json(const ResourceData *data, int count, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Não foi possível abrir o arquivo para escrita JSON");
        return;
    }

    fprintf(fp, "[\n");
    for (int i = 0; i < count; i++) {
        fprintf(fp, "  {\n");
        fprintf(fp, "    \"timestamp\": %ld,\n", data[i].timestamp);
        fprintf(fp, "    \"pid\": %d,\n", data[i].pid);
        fprintf(fp, "    \"cpu_usage_percent\": %.2f,\n", data[i].cpu_usage_percent);
        fprintf(fp, "    \"cpu_user\": %ld,\n", data[i].cpu_user);
        fprintf(fp, "    \"cpu_system\": %ld,\n", data[i].cpu_system);
        fprintf(fp, "    \"num_threads\": %ld,\n", data[i].num_threads);
        fprintf(fp, "    \"voluntary_context_switches\": %ld,\n", data[i].voluntary_context_switches);
        fprintf(fp, "    \"nonvoluntary_context_switches\": %ld,\n", data[i].nonvoluntary_context_switches);
        fprintf(fp, "    \"memory_vsz_kb\": %ld,\n", data[i].memory_vsz);
        fprintf(fp, "    \"memory_rss_pages\": %ld,\n", data[i].memory_rss);
        fprintf(fp, "    \"page_faults_minor\": %ld,\n", data[i].page_faults_minor);
        fprintf(fp, "    \"page_faults_major\": %ld,\n", data[i].page_faults_major);
        fprintf(fp, "    \"memory_swap_kb\": %ld,\n", data[i].memory_swap);
        fprintf(fp, "    \"io_read_bytes\": %lld,\n", data[i].io_read_bytes);
        fprintf(fp, "    \"io_write_bytes\": %lld,\n", data[i].io_write_bytes);
        fprintf(fp, "    \"io_read_rate_bps\": %.2f,\n", data[i].io_read_rate);
        fprintf(fp, "    \"io_write_rate_bps\": %.2f,\n", data[i].io_write_rate);
        fprintf(fp, "    \"io_read_syscalls\": %lld,\n", data[i].io_read_syscalls);
        fprintf(fp, "    \"io_write_syscalls\": %lld,\n", data[i].io_write_syscalls);
        fprintf(fp, "    \"net_rx_bytes\": %lld,\n", data[i].net_rx_bytes);
        fprintf(fp, "    \"net_tx_bytes\": %lld,\n", data[i].net_tx_bytes);
        fprintf(fp, "    \"net_rx_packets\": %lld,\n", data[i].net_rx_packets);
        fprintf(fp, "    \"net_tx_packets\": %lld\n", data[i].net_tx_packets);
        fprintf(fp, "  }%s\n", (i == count - 1) ? "" : ",");
    }
    fprintf(fp, "]\n");

    fclose(fp);
    printf("Dados de monitoramento exportados para '%s'.\n", filename);
}

void export_to_csv(const ResourceData *data, int count, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Não foi possível abrir o arquivo para escrita CSV");
        return;
    }

    // Header
    fprintf(fp, "timestamp,pid,cpu_usage_percent,cpu_user,cpu_system,num_threads,voluntary_context_switches,nonvoluntary_context_switches,memory_vsz_kb,memory_rss_pages,page_faults_minor,page_faults_major,memory_swap_kb,io_read_bytes,io_write_bytes,io_read_rate_bps,io_write_rate_bps,io_read_syscalls,io_write_syscalls,net_rx_bytes,net_tx_bytes,net_rx_packets,net_tx_packets\n");

    // Data
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%ld,%d,%.2f,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%lld,%lld,%.2f,%.2f,%lld,%lld,%lld,%lld,%lld,%lld\n",
                data[i].timestamp,
                data[i].pid,
                data[i].cpu_usage_percent,
                data[i].cpu_user,
                data[i].cpu_system,
                data[i].num_threads,
                data[i].voluntary_context_switches,
                data[i].nonvoluntary_context_switches,
                data[i].memory_vsz,
                data[i].memory_rss,
                data[i].page_faults_minor,
                data[i].page_faults_major,
                data[i].memory_swap,
                data[i].io_read_bytes,
                data[i].io_write_bytes,
                data[i].io_read_rate,
                data[i].io_write_rate,
                data[i].io_read_syscalls,
                data[i].io_write_syscalls,
                data[i].net_rx_bytes,
                data[i].net_tx_bytes,
                data[i].net_rx_packets,
                data[i].net_tx_packets);
    }

    fclose(fp);
    printf("Dados de monitoramento exportados para '%s'.\n", filename);
}
