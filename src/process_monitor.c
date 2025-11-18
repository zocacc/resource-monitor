#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include "../include/process_monitor.h"

#define PROC_PATH_MAX 512

// Variável global para clock ticks por segundo
static long clock_ticks = 0;

// Inicializa clock ticks
static void init_clock_ticks(void) {
    if (clock_ticks == 0) {
        clock_ticks = sysconf(_SC_CLK_TCK);
        if (clock_ticks <= 0) clock_ticks = 100; // Fallback
    }
}

// Obtém nome do processo
bool get_process_name(int pid, char *name, size_t size) {
    char path[PROC_PATH_MAX];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    
    FILE *fp = fopen(path, "r");
    if (!fp) return false;
    
    if (fgets(name, size, fp) == NULL) {
        fclose(fp);
        return false;
    }
    
    // Remove newline
    size_t len = strlen(name);
    if (len > 0 && name[len-1] == '\n') {
        name[len-1] = '\0';
    }
    
    fclose(fp);
    return true;
}

// Coleta métricas de CPU
bool collect_cpu_metrics(int pid, ProcessMetrics *metrics) {
    init_clock_ticks();
    
    char path[PROC_PATH_MAX];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    
    FILE *fp = fopen(path, "r");
    if (!fp) return false;
    
    // Lê dados do /proc/pid/stat
    unsigned long utime, stime;
    int num_threads;
    
    // Formato simplificado - ignoramos campos desnecessários
    char comm[256];
    char state;
    int ppid, pgrp, session, tty_nr, tpgid;
    unsigned long flags, minflt, cminflt, majflt, cmajflt;
    unsigned long cutime, cstime;
    long priority, nice, zero;
    unsigned long itrealvalue, starttime;
    unsigned long vsize;
    long rss;
    
    int ret = fscanf(fp, "%d %s %c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu %lu %lu %ld %ld %ld %d %ld %lu %lu %ld",
                     &metrics->pid, comm, &state, &ppid, &pgrp, &session, &tty_nr, &tpgid,
                     &flags, &minflt, &cminflt, &majflt, &cmajflt,
                     &utime, &stime, &cutime, &cstime,
                     &priority, &nice, &zero, &num_threads, &itrealvalue,
                     &starttime, &vsize, &rss);
    
    fclose(fp);
    
    if (ret < 24) return false;
    
    metrics->cpu_user_time = utime;
    metrics->cpu_system_time = stime;
    metrics->cpu_total_time = utime + stime;
    metrics->num_threads = num_threads;
    metrics->page_faults_minor = minflt;
    metrics->page_faults_major = majflt;
    metrics->mem_vsize = vsize;
    metrics->mem_rss = rss * sysconf(_SC_PAGESIZE);
    
    // Lê context switches de /proc/pid/status
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "voluntary_ctxt_switches: %ld", &metrics->voluntary_ctx_switches) == 1) continue;
            if (sscanf(line, "nonvoluntary_ctxt_switches: %ld", &metrics->nonvoluntary_ctx_switches) == 1) continue;
            if (strncmp(line, "VmSwap:", 7) == 0) {
                sscanf(line, "VmSwap: %ld", &metrics->mem_swap);
            }
        }
        fclose(fp);
    }
    
    return true;
}

// Coleta métricas de memória
bool collect_memory_metrics(int pid, ProcessMetrics *metrics) {
    char path[PROC_PATH_MAX];
    snprintf(path, sizeof(path), "/proc/%d/statm", pid);
    
    FILE *fp = fopen(path, "r");
    if (!fp) return false;
    
    unsigned long size, resident, shared, text, lib, data, dt;
    if (fscanf(fp, "%lu %lu %lu %lu %lu %lu %lu", 
               &size, &resident, &shared, &text, &lib, &data, &dt) != 7) {
        fclose(fp);
        return false;
    }
    
    fclose(fp);
    
    long page_size = sysconf(_SC_PAGESIZE);
    metrics->mem_vsize = size * page_size;
    metrics->mem_rss = resident * page_size;
    metrics->mem_shared = shared * page_size;
    
    return true;
}

// Coleta métricas de I/O
bool collect_io_metrics(int pid, ProcessMetrics *metrics) {
    char path[PROC_PATH_MAX];
    snprintf(path, sizeof(path), "/proc/%d/io", pid);
    
    FILE *fp = fopen(path, "r");
    if (!fp) return false;
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "rchar: %llu", &metrics->io_read_syscalls) == 1) continue;
        if (sscanf(line, "wchar: %llu", &metrics->io_write_syscalls) == 1) continue;
        if (sscanf(line, "read_bytes: %llu", &metrics->io_read_bytes) == 1) continue;
        if (sscanf(line, "write_bytes: %llu", &metrics->io_write_bytes) == 1) continue;
        if (sscanf(line, "cancelled_write_bytes: %llu", &metrics->io_cancelled_write_bytes) == 1) continue;
    }
    
    fclose(fp);
    return true;
}

// Coleta métricas de rede
bool collect_network_metrics(int pid, ProcessMetrics *metrics) {
    // Conta conexões ativas
    char path[PROC_PATH_MAX];
    snprintf(path, sizeof(path), "/proc/%d/net/tcp", pid);
    
    FILE *fp = fopen(path, "r");
    int connections = 0;
    
    if (fp) {
        char line[512];
        fgets(line, sizeof(line), fp); // Skip header
        while (fgets(line, sizeof(line), fp)) {
            connections++;
        }
        fclose(fp);
    }
    
    // TCP6
    snprintf(path, sizeof(path), "/proc/%d/net/tcp6", pid);
    fp = fopen(path, "r");
    if (fp) {
        char line[512];
        fgets(line, sizeof(line), fp); // Skip header
        while (fgets(line, sizeof(line), fp)) {
            connections++;
        }
        fclose(fp);
    }
    
    metrics->net_connections = connections;
    
    // Para métricas de bytes RX/TX, precisaríamos de netlink ou parsing de /proc/net/dev
    // Por simplicidade, vamos coletar do /proc/net/dev global
    fp = fopen("/proc/net/dev", "r");
    if (fp) {
        char line[512];
        // Skip headers
        fgets(line, sizeof(line), fp);
        fgets(line, sizeof(line), fp);
        
        unsigned long long rx_bytes = 0, tx_bytes = 0, rx_packets = 0, tx_packets = 0;
        
        while (fgets(line, sizeof(line), fp)) {
            char iface[32];
            unsigned long long r_bytes, r_packets, r_errs, r_drop, r_fifo, r_frame, r_compressed, r_multicast;
            unsigned long long t_bytes, t_packets, t_errs, t_drop, t_fifo, t_colls, t_carrier, t_compressed;
            
            if (sscanf(line, "%s %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                      iface, &r_bytes, &r_packets, &r_errs, &r_drop, &r_fifo, &r_frame, &r_compressed, &r_multicast,
                      &t_bytes, &t_packets, &t_errs, &t_drop, &t_fifo, &t_colls, &t_carrier, &t_compressed) >= 10) {
                
                // Ignora loopback
                if (strstr(iface, "lo:") == NULL) {
                    rx_bytes += r_bytes;
                    tx_bytes += t_bytes;
                    rx_packets += r_packets;
                    tx_packets += t_packets;
                }
            }
        }
        
        metrics->net_rx_bytes = rx_bytes;
        metrics->net_tx_bytes = tx_bytes;
        metrics->net_rx_packets = rx_packets;
        metrics->net_tx_packets = tx_packets;
        
        fclose(fp);
    }
    
    return true;
}

// Coleta todas as métricas de uma vez
bool collect_process_metrics(int pid, ProcessMetrics *metrics) {
    memset(metrics, 0, sizeof(ProcessMetrics));
    
    metrics->timestamp = time(NULL);
    metrics->pid = pid;
    
    if (!get_process_name(pid, metrics->process_name, sizeof(metrics->process_name))) {
        return false;
    }
    
    bool success = true;
    success &= collect_cpu_metrics(pid, metrics);
    success &= collect_memory_metrics(pid, metrics);
    success &= collect_io_metrics(pid, metrics);
    success &= collect_network_metrics(pid, metrics);
    
    return success;
}

// Calcula CPU% entre duas amostras
void calculate_cpu_percent(ProcessMetrics *current, ProcessMetrics *previous) {
    if (!current || !previous) return;
    
    init_clock_ticks();
    
    unsigned long delta_time = current->cpu_total_time - previous->cpu_total_time;
    time_t delta_sec = current->timestamp - previous->timestamp;
    
    if (delta_sec > 0) {
        // CPU% = (delta_jiffies / ticks_per_sec) / delta_seconds * 100
        current->cpu_usage_percent = (delta_time * 100.0) / (clock_ticks * delta_sec);
    } else {
        current->cpu_usage_percent = 0.0;
    }
}

// Calcula taxas de I/O (KB/s)
void calculate_io_rates(ProcessMetrics *current, ProcessMetrics *previous) {
    if (!current || !previous) return;
    
    time_t delta_sec = current->timestamp - previous->timestamp;
    
    if (delta_sec > 0) {
        unsigned long long delta_read = current->io_read_bytes - previous->io_read_bytes;
        unsigned long long delta_write = current->io_write_bytes - previous->io_write_bytes;
        
        current->io_read_rate_kbs = (delta_read / 1024.0) / delta_sec;
        current->io_write_rate_kbs = (delta_write / 1024.0) / delta_sec;
    } else {
        current->io_read_rate_kbs = 0.0;
        current->io_write_rate_kbs = 0.0;
    }
}

// Gerenciamento de histórico
MetricsHistory* create_metrics_history(int initial_capacity) {
    MetricsHistory *history = malloc(sizeof(MetricsHistory));
    if (!history) return NULL;
    
    history->samples = malloc(sizeof(ProcessMetrics) * initial_capacity);
    if (!history->samples) {
        free(history);
        return NULL;
    }
    
    history->count = 0;
    history->capacity = initial_capacity;
    history->start_time = time(NULL);
    
    return history;
}

void add_metrics_sample(MetricsHistory *history, ProcessMetrics *metrics) {
    if (!history || !metrics) return;
    
    // Expandir se necessário
    if (history->count >= history->capacity) {
        int new_capacity = history->capacity * 2;
        ProcessMetrics *new_samples = realloc(history->samples, sizeof(ProcessMetrics) * new_capacity);
        if (!new_samples) return;
        
        history->samples = new_samples;
        history->capacity = new_capacity;
    }
    
    // Calcular taxas se houver amostra anterior
    if (history->count > 0) {
        ProcessMetrics *prev = &history->samples[history->count - 1];
        calculate_cpu_percent(metrics, prev);
        calculate_io_rates(metrics, prev);
    }
    
    history->samples[history->count++] = *metrics;
}

void free_metrics_history(MetricsHistory *history) {
    if (!history) return;
    free(history->samples);
    free(history);
}

// Exporta para JSON
bool export_metrics_json(const char *filename, MetricsHistory *history) {
    if (!history || history->count == 0) return false;
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Erro ao criar arquivo JSON: %s\n", strerror(errno));
        return false;
    }
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"monitoring_session\": {\n");
    fprintf(fp, "    \"pid\": %d,\n", history->samples[0].pid);
    fprintf(fp, "    \"process_name\": \"%s\",\n", history->samples[0].process_name);
    fprintf(fp, "    \"start_time\": %ld,\n", history->start_time);
    fprintf(fp, "    \"sample_count\": %d\n", history->count);
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"samples\": [\n");
    
    for (int i = 0; i < history->count; i++) {
        ProcessMetrics *m = &history->samples[i];
        
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"timestamp\": %ld,\n", m->timestamp);
        fprintf(fp, "      \"cpu\": {\n");
        fprintf(fp, "        \"user_time\": %lu,\n", m->cpu_user_time);
        fprintf(fp, "        \"system_time\": %lu,\n", m->cpu_system_time);
        fprintf(fp, "        \"usage_percent\": %.2f,\n", m->cpu_usage_percent);
        fprintf(fp, "        \"threads\": %ld,\n", m->num_threads);
        fprintf(fp, "        \"voluntary_ctx_switches\": %ld,\n", m->voluntary_ctx_switches);
        fprintf(fp, "        \"nonvoluntary_ctx_switches\": %ld\n", m->nonvoluntary_ctx_switches);
        fprintf(fp, "      },\n");
        fprintf(fp, "      \"memory\": {\n");
        fprintf(fp, "        \"vsize_bytes\": %ld,\n", m->mem_vsize);
        fprintf(fp, "        \"rss_bytes\": %ld,\n", m->mem_rss);
        fprintf(fp, "        \"shared_bytes\": %ld,\n", m->mem_shared);
        fprintf(fp, "        \"page_faults_minor\": %ld,\n", m->page_faults_minor);
        fprintf(fp, "        \"page_faults_major\": %ld,\n", m->page_faults_major);
        fprintf(fp, "        \"swap_kb\": %ld\n", m->mem_swap);
        fprintf(fp, "      },\n");
        fprintf(fp, "      \"io\": {\n");
        fprintf(fp, "        \"read_bytes\": %llu,\n", m->io_read_bytes);
        fprintf(fp, "        \"write_bytes\": %llu,\n", m->io_write_bytes);
        fprintf(fp, "        \"read_rate_kbs\": %.2f,\n", m->io_read_rate_kbs);
        fprintf(fp, "        \"write_rate_kbs\": %.2f,\n", m->io_write_rate_kbs);
        fprintf(fp, "        \"read_syscalls\": %llu,\n", m->io_read_syscalls);
        fprintf(fp, "        \"write_syscalls\": %llu\n", m->io_write_syscalls);
        fprintf(fp, "      },\n");
        fprintf(fp, "      \"network\": {\n");
        fprintf(fp, "        \"rx_bytes\": %llu,\n", m->net_rx_bytes);
        fprintf(fp, "        \"tx_bytes\": %llu,\n", m->net_tx_bytes);
        fprintf(fp, "        \"rx_packets\": %llu,\n", m->net_rx_packets);
        fprintf(fp, "        \"tx_packets\": %llu,\n", m->net_tx_packets);
        fprintf(fp, "        \"connections\": %d\n", m->net_connections);
        fprintf(fp, "      }\n");
        fprintf(fp, "    }%s\n", (i < history->count - 1) ? "," : "");
    }
    
    fprintf(fp, "  ]\n");
    fprintf(fp, "}\n");
    
    fclose(fp);
    return true;
}

// Exporta para CSV
bool export_metrics_csv(const char *filename, MetricsHistory *history) {
    if (!history || history->count == 0) return false;
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Erro ao criar arquivo CSV: %s\n", strerror(errno));
        return false;
    }
    
    // Header
    fprintf(fp, "timestamp,pid,process_name,");
    fprintf(fp, "cpu_user_time,cpu_system_time,cpu_percent,threads,vol_ctx_switches,nonvol_ctx_switches,");
    fprintf(fp, "mem_vsize,mem_rss,mem_shared,page_faults_minor,page_faults_major,mem_swap_kb,");
    fprintf(fp, "io_read_bytes,io_write_bytes,io_read_kbs,io_write_kbs,io_read_syscalls,io_write_syscalls,");
    fprintf(fp, "net_rx_bytes,net_tx_bytes,net_rx_packets,net_tx_packets,net_connections\n");
    
    // Data
    for (int i = 0; i < history->count; i++) {
        ProcessMetrics *m = &history->samples[i];
        
        fprintf(fp, "%ld,%d,%s,", m->timestamp, m->pid, m->process_name);
        fprintf(fp, "%lu,%lu,%.2f,%ld,%ld,%ld,", 
                m->cpu_user_time, m->cpu_system_time, m->cpu_usage_percent,
                m->num_threads, m->voluntary_ctx_switches, m->nonvoluntary_ctx_switches);
        fprintf(fp, "%ld,%ld,%ld,%ld,%ld,%ld,",
                m->mem_vsize, m->mem_rss, m->mem_shared,
                m->page_faults_minor, m->page_faults_major, m->mem_swap);
        fprintf(fp, "%llu,%llu,%.2f,%.2f,%llu,%llu,",
                m->io_read_bytes, m->io_write_bytes,
                m->io_read_rate_kbs, m->io_write_rate_kbs,
                m->io_read_syscalls, m->io_write_syscalls);
        fprintf(fp, "%llu,%llu,%llu,%llu,%d\n",
                m->net_rx_bytes, m->net_tx_bytes,
                m->net_rx_packets, m->net_tx_packets, m->net_connections);
    }
    
    fclose(fp);
    return true;
}

// Monitoramento contínuo
void monitor_process_continuous(int pid, int interval_sec, int duration_sec, const char *export_format) {
    printf("\n=== Monitoramento de Processo ===\n");
    printf("PID: %d\n", pid);
    printf("Intervalo: %d segundos\n", interval_sec);
    printf("Duração: %d segundos\n", duration_sec);
    printf("Formato de exportação: %s\n\n", export_format);
    
    // Verifica se processo existe
    char comm[256];
    if (!get_process_name(pid, comm, sizeof(comm))) {
        fprintf(stderr, "Erro: Processo com PID %d não encontrado\n", pid);
        return;
    }
    
    printf("Processo: %s\n\n", comm);
    
    int max_samples = (duration_sec / interval_sec) + 1;
    MetricsHistory *history = create_metrics_history(max_samples);
    if (!history) {
        fprintf(stderr, "Erro ao alocar histórico de métricas\n");
        return;
    }
    
    time_t start = time(NULL);
    int sample_num = 0;
    
    printf("%-6s | %-8s | %-10s | %-10s | %-10s | %-10s\n",
           "Sample", "CPU%", "Mem(MB)", "I/O R(KB/s)", "I/O W(KB/s)", "Threads");
    printf("-------|----------|------------|------------|------------|------------\n");
    
    while (1) {
        ProcessMetrics metrics;
        
        if (!collect_process_metrics(pid, &metrics)) {
            fprintf(stderr, "\nProcesso %d terminou ou não pode ser acessado\n", pid);
            break;
        }
        
        add_metrics_sample(history, &metrics);
        sample_num++;
        
        printf("%-6d | %7.2f%% | %10.2f | %10.2f | %10.2f | %10ld\n",
               sample_num,
               metrics.cpu_usage_percent,
               metrics.mem_rss / (1024.0 * 1024.0),
               metrics.io_read_rate_kbs,
               metrics.io_write_rate_kbs,
               metrics.num_threads);
        
        time_t elapsed = time(NULL) - start;
        if (elapsed >= duration_sec) break;
        
        sleep(interval_sec);
    }
    
    printf("\n=== Exportando dados ===\n");
    
    char filename[512];
    bool success = false;
    
    if (strcmp(export_format, "json") == 0) {
        snprintf(filename, sizeof(filename), "output/process_%d_metrics.json", pid);
        success = export_metrics_json(filename, history);
    } else if (strcmp(export_format, "csv") == 0) {
        snprintf(filename, sizeof(filename), "output/process_%d_metrics.csv", pid);
        success = export_metrics_csv(filename, history);
    } else {
        fprintf(stderr, "Formato desconhecido: %s\n", export_format);
    }
    
    if (success) {
        printf("✓ Dados exportados para: %s\n", filename);
        printf("✓ Total de amostras: %d\n", history->count);
    } else {
        fprintf(stderr, "✗ Erro ao exportar dados\n");
    }
    
    free_metrics_history(history);
}

// Interface interativa
void monitor_process_interactive(void) {
    int pid, interval, duration;
    char format[10];
    
    printf("\n=== Monitor de Processo ===\n\n");
    
    printf("Digite o PID do processo: ");
    if (scanf("%d", &pid) != 1 || pid <= 0) {
        printf("PID inválido\n");
        while (getchar() != '\n');
        return;
    }
    
    printf("Intervalo de coleta (segundos): ");
    if (scanf("%d", &interval) != 1 || interval <= 0) {
        printf("Intervalo inválido\n");
        while (getchar() != '\n');
        return;
    }
    
    printf("Duração total (segundos): ");
    if (scanf("%d", &duration) != 1 || duration <= 0) {
        printf("Duração inválida\n");
        while (getchar() != '\n');
        return;
    }
    
    printf("Formato de exportação (json/csv): ");
    if (scanf("%s", format) != 1) {
        printf("Formato inválido\n");
        while (getchar() != '\n');
        return;
    }
    
    while (getchar() != '\n');
    
    monitor_process_continuous(pid, interval, duration, format);
    
    printf("\nPressione ENTER para continuar...");
    getchar();
}
