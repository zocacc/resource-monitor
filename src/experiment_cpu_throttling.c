#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include "../include/utils.h"

#define TEST_DURATION 10
#define NUM_LIMITS 5

// Função auxiliar para obter timestamp ISO8601
static char* get_iso_timestamp() {
    static char buffer[64];
    time_t now = time(NULL);
    struct tm *tm_info = gmtime(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    return buffer;
}

typedef struct {
    double cpu_limit_cores;
    int quota_us;
    int period_us;
    double measured_cpu_percent;
    double deviation_percent;
    long iterations_completed;
    double throughput_iter_per_sec;
    long cpu_usage_us;
    long throttled_us;
    long nr_periods;
    long nr_throttled;
} ThrottleResult;

// Workload CPU-intensive que conta iterações
static long cpu_intensive_workload(int duration_sec) {
    struct timeval start, current;
    gettimeofday(&start, NULL);
    
    long iterations = 0;
    double result = 0.0;
    
    while (1) {
        gettimeofday(&current, NULL);
        long elapsed = (current.tv_sec - start.tv_sec);
        if (elapsed >= duration_sec) break;
        
        // Trabalho CPU intensivo
        for (int i = 0; i < 10000; i++) {
            result += (double)(i * i) / (i + 1.0);
        }
        iterations++;
        
        // Evitar overflow
        if (iterations % 100 == 0) {
            result = result / 2.0;
        }
    }
    
    return iterations;
}

// Executar teste sem limite (baseline)
static ThrottleResult run_baseline_test() {
    ThrottleResult result = {0};
    result.cpu_limit_cores = -1; // Sem limite
    result.quota_us = -1;
    result.period_us = 100000;
    
    printf("      Executando workload... ");
    fflush(stdout);
    
    struct rusage usage_start, usage_end;
    struct timeval time_start, time_end;
    
    getrusage(RUSAGE_SELF, &usage_start);
    gettimeofday(&time_start, NULL);
    
    result.iterations_completed = cpu_intensive_workload(TEST_DURATION);
    
    gettimeofday(&time_end, NULL);
    getrusage(RUSAGE_SELF, &usage_end);
    
    // Calcular tempo real decorrido
    double real_time = (time_end.tv_sec - time_start.tv_sec) + 
                       (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
    
    // Calcular CPU time usado
    long user_us = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) * 1000000 +
                   (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec);
    long sys_us = (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) * 1000000 +
                  (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec);
    
    result.cpu_usage_us = user_us + sys_us;
    
    // CPU% = (cpu_time / real_time) * 100
    result.measured_cpu_percent = (result.cpu_usage_us / 1000000.0 / real_time) * 100.0;
    result.throughput_iter_per_sec = result.iterations_completed / real_time;
    result.deviation_percent = 0.0; // Baseline não tem desvio
    
    printf("Concluido\n");
    printf("      Iteracoes: %ld (%.1f iter/s)\n", 
           result.iterations_completed, result.throughput_iter_per_sec);
    printf("      CPU%%: %.1f%%\n", result.measured_cpu_percent);
    
    return result;
}

// Executar teste com limite de CPU via cgroup
static ThrottleResult run_throttled_test(double cpu_cores, const char *cgroup_name) {
    ThrottleResult result = {0};
    result.cpu_limit_cores = cpu_cores;
    result.period_us = 100000; // 100ms
    result.quota_us = (int)(cpu_cores * result.period_us);
    
    char cgroup_path[256];
    snprintf(cgroup_path, sizeof(cgroup_path), "/sys/fs/cgroup/%s", cgroup_name);
    
    printf("      Criando cgroup e aplicando limite... ");
    fflush(stdout);
    
    // Criar cgroup
    if (mkdir(cgroup_path, 0755) != 0 && errno != EEXIST) {
        printf("ERRO ao criar cgroup\n");
        return result;
    }
    
    // Configurar limite de CPU
    char cpu_max_path[512];
    snprintf(cpu_max_path, sizeof(cpu_max_path), "%s/cpu.max", cgroup_path);
    FILE *f = fopen(cpu_max_path, "w");
    if (f) {
        fprintf(f, "%d %d\n", result.quota_us, result.period_us);
        fclose(f);
    } else {
        printf("ERRO ao configurar cpu.max\n");
        rmdir(cgroup_path);
        return result;
    }
    
    // Adicionar PID atual ao cgroup
    char procs_path[512];
    snprintf(procs_path, sizeof(procs_path), "%s/cgroup.procs", cgroup_path);
    f = fopen(procs_path, "w");
    if (f) {
        fprintf(f, "%d\n", getpid());
        fclose(f);
    }
    
    printf("OK\n");
    printf("      Executando workload... ");
    fflush(stdout);
    
    struct timeval time_start, time_end;
    gettimeofday(&time_start, NULL);
    
    result.iterations_completed = cpu_intensive_workload(TEST_DURATION);
    
    gettimeofday(&time_end, NULL);
    
    double real_time = (time_end.tv_sec - time_start.tv_sec) + 
                       (time_end.tv_usec - time_start.tv_usec) / 1000000.0;
    
    printf("Concluido\n");
    
    // Ler estatísticas do cgroup
    char stat_path[512];
    snprintf(stat_path, sizeof(stat_path), "%s/cpu.stat", cgroup_path);
    f = fopen(stat_path, "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "usage_usec", 10) == 0) {
                sscanf(line, "usage_usec %ld", &result.cpu_usage_us);
            } else if (strncmp(line, "throttled_usec", 14) == 0) {
                sscanf(line, "throttled_usec %ld", &result.throttled_us);
            } else if (strncmp(line, "nr_periods", 10) == 0) {
                sscanf(line, "nr_periods %ld", &result.nr_periods);
            } else if (strncmp(line, "nr_throttled", 12) == 0) {
                sscanf(line, "nr_throttled %ld", &result.nr_throttled);
            }
        }
        fclose(f);
    }
    
    // Mover processo de volta para cgroup raiz
    f = fopen("/sys/fs/cgroup/cgroup.procs", "w");
    if (f) {
        fprintf(f, "%d\n", getpid());
        fclose(f);
    }
    
    // Remover cgroup
    rmdir(cgroup_path);
    
    // Calcular métricas
    result.measured_cpu_percent = (result.cpu_usage_us / 1000000.0 / real_time) * 100.0;
    result.throughput_iter_per_sec = result.iterations_completed / real_time;
    
    double expected_cpu_percent = cpu_cores * 100.0;
    result.deviation_percent = ((result.measured_cpu_percent - expected_cpu_percent) / expected_cpu_percent) * 100.0;
    
    printf("      Iteracoes: %ld (%.1f iter/s)\n", 
           result.iterations_completed, result.throughput_iter_per_sec);
    printf("      CPU%% Medido: %.1f%% (Limite: %.1f%%, Desvio: %+.1f%%)\n", 
           result.measured_cpu_percent, expected_cpu_percent, result.deviation_percent);
    printf("      Throttled: %ld/%ld periodos (%.1f%%)\n",
           result.nr_throttled, result.nr_periods,
           result.nr_periods > 0 ? (result.nr_throttled * 100.0 / result.nr_periods) : 0.0);
    
    return result;
}

void run_experiment_cpu_throttling_new() {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════╗\n");
    printf("║           Experimento 3: Throttling de CPU                           ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Objetivo: Avaliar precisão de limitação de CPU via cgroups\n\n");
    printf("Configuração:\n");
    printf("  • Workload: CPU intensivo com contagem de iterações\n");
    printf("  • Duração: %d segundos por teste\n", TEST_DURATION);
    printf("  • Limites: SEM limite, 0.25, 0.5, 1.0, 2.0 cores\n");
    printf("  • Métricas: CPU%% real, desvio, throughput\n\n");
    
    if (geteuid() != 0) {
        fprintf(stderr, "ERRO: Este experimento requer privilégios root.\n");
        fprintf(stderr, "Execute com: sudo ./bin/monitor experiment 3\n");
        return;
    }
    
    // Habilitar controlador CPU no cgroup raiz
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("PREPARAÇÃO: Habilitando controlador de CPU\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    FILE *f = fopen("/sys/fs/cgroup/cgroup.subtree_control", "w");
    if (f) {
        fprintf(f, "+cpu\n");
        fclose(f);
        printf("✓ Controlador CPU habilitado\n\n");
    }
    
    ThrottleResult results[NUM_LIMITS];
    double limits[NUM_LIMITS] = {-1, 0.25, 0.5, 1.0, 2.0}; // -1 = sem limite
    
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("FASE 1: Baseline (SEM limite de CPU)\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    
    results[0] = run_baseline_test();
    printf("\n");
    
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("FASE 2: Testes COM limitação de CPU\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    for (int i = 1; i < NUM_LIMITS; i++) {
        char cgroup_name[64];
        snprintf(cgroup_name, sizeof(cgroup_name), "exp3_cpu_%.2f_%d", limits[i], getpid());
        
        printf("Teste %d/%d: Limite de %.2f cores\n", i, NUM_LIMITS-1, limits[i]);
        results[i] = run_throttled_test(limits[i], cgroup_name);
        printf("\n");
        sleep(1); // Pequena pausa entre testes
    }
    
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("RESULTADOS: Análise de Throttling\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    printf("%-12s | %-12s | %-12s | %-12s | %-15s\n",
           "Limite", "CPU% Medido", "Desvio(%)", "Throughput", "% do Baseline");
    printf("-------------|--------------|--------------|--------------|----------------\n");
    
    // Baseline
    printf("%-12s | %12.1f | %12s | %12.1f | %15s\n",
           "Sem limite", 
           results[0].measured_cpu_percent,
           "-",
           results[0].throughput_iter_per_sec,
           "100.0%");
    
    // Com limites
    for (int i = 1; i < NUM_LIMITS; i++) {
        double throughput_ratio = (results[i].throughput_iter_per_sec / results[0].throughput_iter_per_sec) * 100.0;
        
        char limit_str[16];
        snprintf(limit_str, sizeof(limit_str), "%.2f cores", limits[i]);
        
        printf("%-12s | %12.1f | %12.1f | %12.1f | %15.1f\n",
               limit_str,
               results[i].measured_cpu_percent,
               results[i].deviation_percent,
               results[i].throughput_iter_per_sec,
               throughput_ratio);
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("ANÁLISE E CONCLUSÕES\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    // Calcular desvio médio
    double avg_deviation = 0.0;
    for (int i = 1; i < NUM_LIMITS; i++) {
        avg_deviation += fabs(results[i].deviation_percent);
    }
    avg_deviation /= (NUM_LIMITS - 1);
    
    printf("Precisão do Throttling:\n");
    printf("   • Desvio médio: %.2f%%\n", avg_deviation);
    printf("\n");
    
    printf("Observações:\n");
    printf("   • CPU%% medido vs configurado mostra eficácia do cgroup\n");
    printf("   • Throughput é diretamente proporcional ao limite de CPU\n");
    printf("   • Desvios indicam precisão do controle do kernel\n");
    printf("\n");
    
    printf("Recomendações:\n");
    if (avg_deviation < 5.0) {
        printf("   • ALTA precisão - cgroups v2 limitam CPU com precisão\n");
        printf("   • Desvio < 5%% é excelente para controle de recursos\n");
    } else if (avg_deviation < 15.0) {
        printf("   • MÉDIA precisão - aceitável para a maioria dos casos\n");
        printf("   • Considere ajustes finos se necessário controle rigoroso\n");
    } else {
        printf("   • BAIXA precisão - verificar configuração do sistema\n");
        printf("   • Pode haver contenção ou outros fatores\n");
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    
    // Exportar resultados
    printf("\nExportando resultados para arquivo...\n");
    
    FILE *csv = fopen("output/experiment3_cpu_throttling.csv", "w");
    if (csv) {
        fprintf(csv, "cpu_limit_cores,quota_us,period_us,measured_cpu_percent,");
        fprintf(csv, "deviation_percent,iterations_completed,throughput_iter_per_sec,");
        fprintf(csv, "cpu_usage_us,throttled_us,nr_periods,nr_throttled,throttle_rate_percent\n");
        
        for (int i = 0; i < NUM_LIMITS; i++) {
            double throttle_rate = results[i].nr_periods > 0 ? 
                                  (results[i].nr_throttled * 100.0 / results[i].nr_periods) : 0.0;
            
            fprintf(csv, "%.2f,%d,%d,%.2f,%.2f,%ld,%.2f,%ld,%ld,%ld,%ld,%.2f\n",
                    results[i].cpu_limit_cores,
                    results[i].quota_us,
                    results[i].period_us,
                    results[i].measured_cpu_percent,
                    results[i].deviation_percent,
                    results[i].iterations_completed,
                    results[i].throughput_iter_per_sec,
                    results[i].cpu_usage_us,
                    results[i].throttled_us,
                    results[i].nr_periods,
                    results[i].nr_throttled,
                    throttle_rate);
        }
        
        fclose(csv);
        printf("✓ Resultados salvos em: output/experiment3_cpu_throttling.csv\n");
    } else {
        printf("✗ Erro ao criar arquivo CSV\n");
    }
    
    // Exportar JSON também
    FILE *json = fopen("output/experiments/exp3_cpu_throttling.json", "w");
    if (json) {
        fprintf(json, "{\n");
        fprintf(json, "  \"experiment\": \"CPU Throttling with Cgroups v2\",\n");
        fprintf(json, "  \"date\": \"%s\",\n", get_iso_timestamp());
        fprintf(json, "  \"configuration\": {\n");
        fprintf(json, "    \"test_duration_sec\": %d,\n", TEST_DURATION);
        fprintf(json, "    \"num_tests\": %d\n", NUM_LIMITS);
        fprintf(json, "  },\n");
        fprintf(json, "  \"results\": [\n");
        
        for (int i = 0; i < NUM_LIMITS; i++) {
            fprintf(json, "    {\n");
            fprintf(json, "      \"cpu_limit_cores\": %.2f,\n", results[i].cpu_limit_cores);
            fprintf(json, "      \"measured_cpu_percent\": %.2f,\n", results[i].measured_cpu_percent);
            fprintf(json, "      \"deviation_percent\": %.2f,\n", results[i].deviation_percent);
            fprintf(json, "      \"throughput_iter_per_sec\": %.2f,\n", results[i].throughput_iter_per_sec);
            fprintf(json, "      \"iterations_completed\": %ld,\n", results[i].iterations_completed);
            fprintf(json, "      \"throttled_periods\": %ld,\n", results[i].nr_throttled);
            fprintf(json, "      \"total_periods\": %ld\n", results[i].nr_periods);
            fprintf(json, "    }%s\n", i < NUM_LIMITS - 1 ? "," : "");
        }
        
        fprintf(json, "  ],\n");
        fprintf(json, "  \"analysis\": {\n");
        fprintf(json, "    \"average_deviation_percent\": %.2f\n", avg_deviation);
        fprintf(json, "  }\n");
        fprintf(json, "}\n");
        fclose(json);
        printf("✓ Relatório JSON salvo em: output/experiments/exp3_cpu_throttling.json\n");
    }
    
    printf("\n");
}
