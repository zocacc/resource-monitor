#ifndef MONITOR_H
#define MONITOR_H

#include <time.h>
#include <stdbool.h>

// Estrutura para armazenar os dados de monitoramento de recursos
typedef struct {
    // Timestamp
    time_t timestamp;
    int pid;

    // CPU
    long cpu_user;
    long cpu_system;
    long num_threads;
    long voluntary_context_switches;
    long nonvoluntary_context_switches;
    double cpu_usage_percent; // Calculado

    // Memória
    long memory_vsz; // Virtual Size in KB
    long memory_rss; // Resident Size in pages
    long page_faults_minor;
    long page_faults_major;
    long memory_swap; // Swap usage in KB

    // I/O
    long long io_read_bytes;
    long long io_write_bytes;
    double io_read_rate;  // Calculado (bytes/sec)
    double io_write_rate; // Calculado (bytes/sec)
    long long io_read_syscalls;
    long long io_write_syscalls;

    // Rede
    long long net_rx_bytes;
    long long net_tx_bytes;
    long long net_rx_packets;
    long long net_tx_packets;

} ResourceData;

// Funções para coleta de dados. Retornam 'true' em sucesso, 'false' em falha.
bool get_cpu_data(int pid, ResourceData *data);
bool get_memory_data(int pid, ResourceData *data);
bool get_io_data(int pid, ResourceData *data);

#endif // MONITOR_H
