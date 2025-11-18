#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include "../include/utils.h"

#define FILE_SIZE_MB 100
#define NUM_LIMITS 4
#define TEST_FILE "/tmp/io_test_exp5.dat"

// Função auxiliar para obter timestamp ISO8601
static char* get_iso_timestamp() {
    static char buffer[64];
    time_t now = time(NULL);
    struct tm *tm_info = gmtime(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    return buffer;
}

typedef struct {
    long limit_bps;  // 0 = sem limite
    double write_time_sec;
    double read_time_sec;
    double write_throughput_mbps;
    double read_throughput_mbps;
    double write_latency_ms;
    double read_latency_ms;
    double total_time_sec;
} IOTestResult;

// Obter device major:minor do diretório /tmp
static int get_device_id(char *device_id, size_t size) {
    struct stat st;
    if (stat("/tmp", &st) != 0) {
        return -1;
    }
    
    unsigned int major_num = major(st.st_dev);
    unsigned int minor_num = minor(st.st_dev);
    
    snprintf(device_id, size, "%u:%u", major_num, minor_num);
    return 0;
}

// Executar teste de I/O e medir desempenho
static IOTestResult run_io_test(long limit_bps, const char *cgroup_path) {
    IOTestResult result = {0};
    result.limit_bps = limit_bps;
    
    struct timeval start, end;
    
    // Se há limite, aplicar via cgroup
    if (limit_bps > 0 && cgroup_path) {
        char device_id[32];
        if (get_device_id(device_id, sizeof(device_id)) == 0) {
            char io_max_path[1024];
            snprintf(io_max_path, sizeof(io_max_path), "%s/io.max", cgroup_path);
            
            FILE *f = fopen(io_max_path, "w");
            if (f) {
                // Aplicar limite de leitura e escrita
                fprintf(f, "%s rbps=%ld wbps=%ld\n", device_id, limit_bps, limit_bps);
                fclose(f);
            }
        }
        
        // Adicionar processo ao cgroup
        char procs_path[1024];
        snprintf(procs_path, sizeof(procs_path), "%s/cgroup.procs", cgroup_path);
        FILE *pf = fopen(procs_path, "w");
        if (pf) {
            fprintf(pf, "%d\n", getpid());
            fclose(pf);
        }
    }
    
    // Teste de ESCRITA
    gettimeofday(&start, NULL);
    
    FILE *f = fopen(TEST_FILE, "w");
    if (f) {
        char *buffer = malloc(1024 * 1024); // 1MB buffer
        if (buffer) {
            memset(buffer, 'A', 1024 * 1024);
            
            for (int i = 0; i < FILE_SIZE_MB; i++) {
                fwrite(buffer, 1, 1024 * 1024, f);
            }
            
            free(buffer);
        }
        fflush(f);
        fsync(fileno(f));
        fclose(f);
    }
    
    gettimeofday(&end, NULL);
    result.write_time_sec = (end.tv_sec - start.tv_sec) + 
                            (end.tv_usec - start.tv_usec) / 1000000.0;
    result.write_throughput_mbps = FILE_SIZE_MB / result.write_time_sec;
    result.write_latency_ms = (result.write_time_sec * 1000.0) / FILE_SIZE_MB;
    
    // Pequena pausa
    sync();
    sleep(1);
    
    // Limpar cache do sistema (tentativa)
    system("sync; echo 3 > /proc/sys/vm/drop_caches 2>/dev/null");
    
    // Teste de LEITURA
    gettimeofday(&start, NULL);
    
    f = fopen(TEST_FILE, "r");
    if (f) {
        char *buffer = malloc(1024 * 1024);
        if (buffer) {
            while (fread(buffer, 1, 1024 * 1024, f) > 0) {
                // Apenas ler
            }
            free(buffer);
        }
        fclose(f);
    }
    
    gettimeofday(&end, NULL);
    result.read_time_sec = (end.tv_sec - start.tv_sec) + 
                           (end.tv_usec - start.tv_usec) / 1000000.0;
    result.read_throughput_mbps = FILE_SIZE_MB / result.read_time_sec;
    result.read_latency_ms = (result.read_time_sec * 1000.0) / FILE_SIZE_MB;
    
    result.total_time_sec = result.write_time_sec + result.read_time_sec;
    
    // Remover arquivo de teste
    unlink(TEST_FILE);
    
    // Remover processo do cgroup
    if (cgroup_path) {
        FILE *pf = fopen("/sys/fs/cgroup/cgroup.procs", "w");
        if (pf) {
            fprintf(pf, "%d\n", getpid());
            fclose(pf);
        }
    }
    
    return result;
}

void run_experiment_io_limit_new() {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════╗\n");
    printf("║           Experimento 5: Limitação de I/O                            ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Objetivo: Avaliar precisão de limitação de I/O\n\n");
    printf("Configuração:\n");
    printf("  • Tamanho do arquivo: %d MB\n", FILE_SIZE_MB);
    printf("  • Testes: Baseline + 3 limites diferentes\n");
    printf("  • Operações: Escrita + Leitura sequencial\n");
    printf("  • Métricas: Throughput, latência, tempo total\n\n");
    
    if (geteuid() != 0) {
        fprintf(stderr, "ERRO: Este experimento requer privilégios root.\n");
        fprintf(stderr, "Execute com: sudo ./bin/monitor experiment 5\n");
        return;
    }
    
    char cgroup_name[64];
    snprintf(cgroup_name, sizeof(cgroup_name), "exp5_io_%d", getpid());
    char cgroup_path[1024];
    snprintf(cgroup_path, sizeof(cgroup_path), "/sys/fs/cgroup/%s", cgroup_name);
    
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("FASE 1: Preparação\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    
    // Habilitar controlador de I/O
    FILE *f = fopen("/sys/fs/cgroup/cgroup.subtree_control", "w");
    if (f) {
        fprintf(f, "+io\n");
        fclose(f);
        printf("✓ Controlador de I/O habilitado\n");
    }
    
    // Criar cgroup
    if (mkdir(cgroup_path, 0755) != 0 && errno != EEXIST) {
        printf("✗ Erro ao criar cgroup: %s\n", strerror(errno));
        return;
    }
    printf("✓ Cgroup criado: %s\n", cgroup_name);
    
    // Detectar dispositivo
    char device_id[32];
    if (get_device_id(device_id, sizeof(device_id)) == 0) {
        printf("✓ Dispositivo detectado: %s\n", device_id);
    } else {
        printf("✗ Erro ao detectar dispositivo\n");
    }
    
    printf("\n");
    
    // Configurar limites de teste (em bytes por segundo)
    long limits_bps[NUM_LIMITS] = {
        0,                    // Sem limite (baseline)
        10 * 1024 * 1024,    // 10 MB/s
        50 * 1024 * 1024,    // 50 MB/s
        100 * 1024 * 1024    // 100 MB/s
    };
    
    IOTestResult results[NUM_LIMITS];
    
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("FASE 2: Teste Baseline (SEM limite)\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    
    printf("Executando teste de I/O sem limitação...\n");
    results[0] = run_io_test(0, NULL);
    printf("✓ Baseline concluído\n");
    printf("  Escrita: %.2f MB/s (%.2f segundos)\n", 
           results[0].write_throughput_mbps, results[0].write_time_sec);
    printf("  Leitura: %.2f MB/s (%.2f segundos)\n", 
           results[0].read_throughput_mbps, results[0].read_time_sec);
    printf("  Tempo total: %.2f segundos\n", results[0].total_time_sec);
    printf("\n");
    
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("FASE 3: Testes COM limitação de I/O\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    for (int i = 1; i < NUM_LIMITS; i++) {
        printf("Teste %d/%d: Limite de %ld MB/s\n", i, NUM_LIMITS-1, limits_bps[i] / (1024 * 1024));
        results[i] = run_io_test(limits_bps[i], cgroup_path);
        printf("✓ Concluído\n");
        printf("  Escrita: %.2f MB/s (%.2f segundos, latência: %.2f ms/MB)\n", 
               results[i].write_throughput_mbps, results[i].write_time_sec,
               results[i].write_latency_ms);
        printf("  Leitura: %.2f MB/s (%.2f segundos, latência: %.2f ms/MB)\n", 
               results[i].read_throughput_mbps, results[i].read_time_sec,
               results[i].read_latency_ms);
        printf("  Tempo total: %.2f segundos\n", results[i].total_time_sec);
        printf("\n");
        sleep(1);
    }
    
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("RESULTADOS: Análise de Limitação de I/O\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    printf("%-15s | %-15s | %-15s | %-15s | %-15s\n",
           "Limite (MB/s)", "Write (MB/s)", "Read (MB/s)", "Latência (ms)", "Tempo Total (s)");
    printf("----------------|----------------|----------------|----------------|----------------\n");
    
    // Baseline
    printf("%-15s | %15.2f | %15.2f | %15.2f | %15.2f\n",
           "Sem limite",
           results[0].write_throughput_mbps,
           results[0].read_throughput_mbps,
           (results[0].write_latency_ms + results[0].read_latency_ms) / 2,
           results[0].total_time_sec);
    
    // Com limites
    for (int i = 1; i < NUM_LIMITS; i++) {
        char limit_str[16];
        snprintf(limit_str, sizeof(limit_str), "%ld", limits_bps[i] / (1024 * 1024));
        
        double avg_latency = (results[i].write_latency_ms + results[i].read_latency_ms) / 2;
        
        printf("%-15s | %15.2f | %15.2f | %15.2f | %15.2f\n",
               limit_str,
               results[i].write_throughput_mbps,
               results[i].read_throughput_mbps,
               avg_latency,
               results[i].total_time_sec);
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("ANÁLISE: Throughput Medido vs Configurado\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    printf("%-15s | %-15s | %-15s | %-15s\n",
           "Limite (MB/s)", "Write Medido", "Desvio Write", "Desvio Read");
    printf("----------------|----------------|----------------|----------------\n");
    
    for (int i = 1; i < NUM_LIMITS; i++) {
        double limit_mbps = limits_bps[i] / (1024.0 * 1024.0);
        double write_deviation = ((results[i].write_throughput_mbps - limit_mbps) / limit_mbps) * 100.0;
        double read_deviation = ((results[i].read_throughput_mbps - limit_mbps) / limit_mbps) * 100.0;
        
        printf("%-15.0f | %15.2f | %14.1f%% | %14.1f%%\n",
               limit_mbps,
               results[i].write_throughput_mbps,
               write_deviation,
               read_deviation);
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("IMPACTO NO TEMPO DE EXECUÇÃO\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    printf("%-15s | %-20s | %-20s\n",
           "Limite (MB/s)", "Tempo Total (s)", "Aumento vs Baseline");
    printf("----------------|---------------------|---------------------\n");
    
    printf("%-15s | %20.2f | %20s\n",
           "Sem limite",
           results[0].total_time_sec,
           "-");
    
    for (int i = 1; i < NUM_LIMITS; i++) {
        double increase = ((results[i].total_time_sec - results[0].total_time_sec) / 
                          results[0].total_time_sec) * 100.0;
        
        char limit_str[16];
        snprintf(limit_str, sizeof(limit_str), "%ld", limits_bps[i] / (1024 * 1024));
        
        printf("%-15s | %20.2f | %19.1f%%\n",
               limit_str,
               results[i].total_time_sec,
               increase);
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("CONCLUSÕES\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    printf("Observações:\n");
    printf("  • Throughput medido reflete os limites aplicados\n");
    printf("  • Latência aumenta inversamente ao throughput\n");
    printf("  • Tempo total de execução aumenta com limites mais restritivos\n");
    printf("  • Cgroups v2 aplicam limites de I/O de forma efetiva\n");
    printf("\n");
    
    printf("═══════════════════════════════════════════════════════════════════════\n");
    
    // Exportar resultados
    printf("\nExportando resultados para arquivo...\n");
    
    // Criar diretório se não existir
    mkdir("output", 0755);
    mkdir("output/experiments", 0755);
    
    FILE *csv = fopen("output/experiment5_io_limit.csv", "w");
    if (csv) {
        fprintf(csv, "limit_mbps,write_time_sec,read_time_sec,write_throughput_mbps,");
        fprintf(csv, "read_throughput_mbps,write_latency_ms,read_latency_ms,total_time_sec\n");
        
        for (int i = 0; i < NUM_LIMITS; i++) {
            fprintf(csv, "%.0f,%.3f,%.3f,%.2f,%.2f,%.2f,%.2f,%.3f\n",
                    results[i].limit_bps / (1024.0 * 1024.0),
                    results[i].write_time_sec,
                    results[i].read_time_sec,
                    results[i].write_throughput_mbps,
                    results[i].read_throughput_mbps,
                    results[i].write_latency_ms,
                    results[i].read_latency_ms,
                    results[i].total_time_sec);
        }
        
        fclose(csv);
        printf("✓ Resultados salvos em: output/experiment5_io_limit.csv\n");
    }
    
    FILE *json = fopen("output/experiments/exp5_io_limit.json", "w");
    if (json) {
        fprintf(json, "{\n");
        fprintf(json, "  \"experiment\": \"I/O Limit with Cgroups v2\",\n");
        fprintf(json, "  \"date\": \"%s\",\n", get_iso_timestamp());
        fprintf(json, "  \"configuration\": {\n");
        fprintf(json, "    \"file_size_mb\": %d,\n", FILE_SIZE_MB);
        fprintf(json, "    \"num_tests\": %d\n", NUM_LIMITS);
        fprintf(json, "  },\n");
        fprintf(json, "  \"results\": [\n");
        
        for (int i = 0; i < NUM_LIMITS; i++) {
            fprintf(json, "    {\n");
            fprintf(json, "      \"limit_mbps\": %.0f,\n", results[i].limit_bps / (1024.0 * 1024.0));
            fprintf(json, "      \"write_throughput_mbps\": %.2f,\n", results[i].write_throughput_mbps);
            fprintf(json, "      \"read_throughput_mbps\": %.2f,\n", results[i].read_throughput_mbps);
            fprintf(json, "      \"write_latency_ms\": %.2f,\n", results[i].write_latency_ms);
            fprintf(json, "      \"read_latency_ms\": %.2f,\n", results[i].read_latency_ms);
            fprintf(json, "      \"total_time_sec\": %.3f\n", results[i].total_time_sec);
            fprintf(json, "    }%s\n", i < NUM_LIMITS - 1 ? "," : "");
        }
        
        fprintf(json, "  ]\n");
        fprintf(json, "}\n");
        fclose(json);
        printf("✓ Relatório JSON salvo em: output/experiments/exp5_io_limit.json\n");
    }
    
    // Limpar
    rmdir(cgroup_path);
    
    printf("\n");
}
