#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <time.h>
#include <stdbool.h>

// Estrutura detalhada de métricas de processo
typedef struct {
    time_t timestamp;
    int pid;
    char process_name[256];
    
    // CPU
    unsigned long cpu_user_time;       // User mode time (jiffies)
    unsigned long cpu_system_time;     // Kernel mode time (jiffies)
    unsigned long cpu_total_time;      // Total time
    double cpu_usage_percent;          // CPU% calculado
    long num_threads;
    long voluntary_ctx_switches;
    long nonvoluntary_ctx_switches;
    
    // Memória
    long mem_vsize;                    // Virtual memory size (bytes)
    long mem_rss;                      // Resident Set Size (bytes)
    long mem_shared;                   // Shared memory (bytes)
    long page_faults_minor;
    long page_faults_major;
    long mem_swap;                     // Swap usage (KB)
    
    // I/O
    unsigned long long io_read_bytes;
    unsigned long long io_write_bytes;
    unsigned long long io_cancelled_write_bytes;
    double io_read_rate_kbs;           // KB/s
    double io_write_rate_kbs;          // KB/s
    unsigned long long io_read_syscalls;
    unsigned long long io_write_syscalls;
    
    // Rede
    unsigned long long net_rx_bytes;
    unsigned long long net_tx_bytes;
    unsigned long long net_rx_packets;
    unsigned long long net_tx_packets;
    int net_connections;
    
} ProcessMetrics;

// Histórico de métricas para cálculo de taxas
typedef struct {
    ProcessMetrics *samples;
    int count;
    int capacity;
    time_t start_time;
} MetricsHistory;

// Funções de coleta
bool collect_process_metrics(int pid, ProcessMetrics *metrics);
bool collect_cpu_metrics(int pid, ProcessMetrics *metrics);
bool collect_memory_metrics(int pid, ProcessMetrics *metrics);
bool collect_io_metrics(int pid, ProcessMetrics *metrics);
bool collect_network_metrics(int pid, ProcessMetrics *metrics);

// Funções de cálculo
void calculate_cpu_percent(ProcessMetrics *current, ProcessMetrics *previous);
void calculate_io_rates(ProcessMetrics *current, ProcessMetrics *previous);

// Funções de monitoramento contínuo
void monitor_process_continuous(int pid, int interval_sec, int duration_sec, const char *export_format);
void monitor_process_interactive(void);

// Funções de exportação
bool export_metrics_json(const char *filename, MetricsHistory *history);
bool export_metrics_csv(const char *filename, MetricsHistory *history);

// Gerenciamento de histórico
MetricsHistory* create_metrics_history(int initial_capacity);
void add_metrics_sample(MetricsHistory *history, ProcessMetrics *metrics);
void free_metrics_history(MetricsHistory *history);

// Função auxiliar para obter nome do processo
bool get_process_name(int pid, char *name, size_t size);

#endif // PROCESS_MONITOR_H
