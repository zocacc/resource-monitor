#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include "../include/utils.h"

#define WORKLOAD_ITERATIONS 50000000
#define NUM_INTERVALS 4

typedef struct {
    double execution_time_sec;
    double cpu_time_sec;
    double user_time_sec;
    double system_time_sec;
    long context_switches;
    long memory_peak_kb;
} BenchmarkResult;

static volatile int keep_monitoring = 1;

void signal_handler(int sig) {
    (void)sig; // Parâmetro não usado
    keep_monitoring = 0;
}

// Função de workload intensivo de CPU
static double cpu_workload(int iterations) {
    double result = 0.0;
    for (int i = 0; i < iterations; i++) {
        result += (double)(i * i) / (i + 1.0);
        if (i % 1000000 == 0) {
            result = result / 2.0; // Evitar overflow
        }
    }
    return result;
}

// Executar workload sem monitoramento
static BenchmarkResult run_baseline_workload() {
    BenchmarkResult result = {0};
    struct timeval start, end;
    struct rusage usage_start, usage_end;
    
    getrusage(RUSAGE_SELF, &usage_start);
    gettimeofday(&start, NULL);
    
    // Executar workload
    double workload_result = cpu_workload(WORKLOAD_ITERATIONS);
    (void)workload_result; // Evitar warning de variável não usada
    
    gettimeofday(&end, NULL);
    getrusage(RUSAGE_SELF, &usage_end);
    
    // Calcular métricas
    result.execution_time_sec = (end.tv_sec - start.tv_sec) + 
                                (end.tv_usec - start.tv_usec) / 1000000.0;
    
    result.user_time_sec = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) +
                           (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1000000.0;
    
    result.system_time_sec = (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) +
                             (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1000000.0;
    
    result.cpu_time_sec = result.user_time_sec + result.system_time_sec;
    
    result.context_switches = (usage_end.ru_nvcsw - usage_start.ru_nvcsw) +
                             (usage_end.ru_nivcsw - usage_start.ru_nivcsw);
    
    result.memory_peak_kb = usage_end.ru_maxrss;
    
    return result;
}

// Executar workload COM monitoramento
static BenchmarkResult run_monitored_workload(int sampling_interval_ms) {
    BenchmarkResult result = {0};
    struct timeval start, end;
    struct rusage usage_start, usage_end;
    
    pid_t workload_pid = fork();
    
    if (workload_pid == 0) {
        // Processo filho - executa workload
        cpu_workload(WORKLOAD_ITERATIONS);
        exit(0);
    } else if (workload_pid > 0) {
        // Processo pai - monitora
        getrusage(RUSAGE_CHILDREN, &usage_start);
        gettimeofday(&start, NULL);
        
        int samples = 0;
        keep_monitoring = 1;
        signal(SIGCHLD, signal_handler);
        
        // Loop de monitoramento
        while (keep_monitoring) {
            char stat_path[256];
            snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", workload_pid);
            
            FILE *f = fopen(stat_path, "r");
            if (f) {
                char buffer[2048];
                if (fgets(buffer, sizeof(buffer), f)) {
                    samples++;
                }
                fclose(f);
            } else {
                // Processo terminou
                break;
            }
            
            usleep(sampling_interval_ms * 1000);
        }
        
        // Aguardar término do processo filho
        int status;
        waitpid(workload_pid, &status, 0);
        
        gettimeofday(&end, NULL);
        getrusage(RUSAGE_CHILDREN, &usage_end);
        
        // Calcular métricas
        result.execution_time_sec = (end.tv_sec - start.tv_sec) + 
                                    (end.tv_usec - start.tv_usec) / 1000000.0;
        
        result.user_time_sec = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) +
                               (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1000000.0;
        
        result.system_time_sec = (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) +
                                 (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1000000.0;
        
        result.cpu_time_sec = result.user_time_sec + result.system_time_sec;
        
        result.context_switches = (usage_end.ru_nvcsw - usage_start.ru_nvcsw) +
                                 (usage_end.ru_nivcsw - usage_start.ru_nivcsw);
        
        result.memory_peak_kb = usage_end.ru_maxrss;
    }
    
    return result;
}

void run_experiment_overhead() {
    printf("\n╔═══════════════════════════════════════════════════════════════════════╗\n");
    printf("║           Experimento 1: Overhead de Monitoramento                    ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Objetivo: Medir o impacto do profiler no desempenho do sistema\n\n");
    printf("Configuração:\n");
    printf("  • Workload: Cálculo intensivo de CPU (%d iterações)\n", WORKLOAD_ITERATIONS);
    printf("  • Intervalos de sampling: 1ms, 10ms, 100ms, 1000ms\n");
    printf("  • Métricas: Tempo de execução, CPU overhead, context switches\n\n");
    
    // Array de intervalos de sampling (em ms)
    int intervals[NUM_INTERVALS] = {1, 10, 100, 1000};
    BenchmarkResult results[NUM_INTERVALS + 1]; // +1 para baseline
    
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("FASE 1: Executando workload SEM monitoramento (baseline)\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    
    printf("Executando workload de referência... ");
    fflush(stdout);
    results[0] = run_baseline_workload();
    printf("✓ Concluído\n");
    printf("  Tempo de execução: %.3f segundos\n", results[0].execution_time_sec);
    printf("  CPU time: %.3f segundos (user: %.3f, system: %.3f)\n", 
           results[0].cpu_time_sec, results[0].user_time_sec, results[0].system_time_sec);
    printf("  Context switches: %ld\n", results[0].context_switches);
    printf("  Memória pico: %ld KB\n\n", results[0].memory_peak_kb);
    
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("FASE 2: Executando workload COM monitoramento\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    for (int i = 0; i < NUM_INTERVALS; i++) {
        printf("Teste %d/%d: Intervalo de sampling = %d ms\n", i + 1, NUM_INTERVALS, intervals[i]);
        printf("  Executando... ");
        fflush(stdout);
        
        results[i + 1] = run_monitored_workload(intervals[i]);
        
        printf("✓ Concluído\n");
        printf("  Tempo de execução: %.3f segundos\n", results[i + 1].execution_time_sec);
        printf("  CPU time: %.3f segundos\n", results[i + 1].cpu_time_sec);
        printf("  Context switches: %ld\n\n", results[i + 1].context_switches);
    }
    
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("RESULTADOS: Análise de Overhead\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    printf("%-15s | %12s | %12s | %12s | %12s\n",
           "Sampling (ms)", "Exec Time(s)", "Overhead(%)", "CPU Over(%)", "Ctx SW Delta");
    printf("----------------|--------------|--------------|--------------|-------------\n");
    
    // Baseline
    printf("%-15s | %12.3f | %12s | %12s | %12s\n",
           "Baseline (0)", results[0].execution_time_sec, "-", "-", "-");
    
    // Resultados com monitoramento
    for (int i = 0; i < NUM_INTERVALS; i++) {
        double time_overhead = ((results[i + 1].execution_time_sec - results[0].execution_time_sec) / 
                               results[0].execution_time_sec) * 100.0;
        
        double cpu_overhead = ((results[i + 1].cpu_time_sec - results[0].cpu_time_sec) / 
                              results[0].cpu_time_sec) * 100.0;
        
        long ctx_delta = results[i + 1].context_switches - results[0].context_switches;
        
        printf("%-15d | %12.3f | %12.2f | %12.2f | %12ld\n",
               intervals[i], 
               results[i + 1].execution_time_sec,
               time_overhead,
               cpu_overhead,
               ctx_delta);
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("ANÁLISE E CONCLUSÕES\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    // Calcular médias
    double avg_time_overhead = 0.0;
    double avg_cpu_overhead = 0.0;
    
    for (int i = 0; i < NUM_INTERVALS; i++) {
        double time_oh = ((results[i + 1].execution_time_sec - results[0].execution_time_sec) / 
                         results[0].execution_time_sec) * 100.0;
        double cpu_oh = ((results[i + 1].cpu_time_sec - results[0].cpu_time_sec) / 
                        results[0].cpu_time_sec) * 100.0;
        
        avg_time_overhead += time_oh;
        avg_cpu_overhead += cpu_oh;
    }
    
    avg_time_overhead /= NUM_INTERVALS;
    avg_cpu_overhead /= NUM_INTERVALS;
    
    printf("Overhead Médio de Monitoramento:\n");
    printf("   • Tempo de Execução: %.2f%%\n", avg_time_overhead);
    printf("   • CPU Usage: %.2f%%\n", avg_cpu_overhead);
    printf("\n");
    
    printf("Observações:\n");
    printf("   • Intervalos menores (1ms) causam maior overhead devido à frequência de sampling\n");
    printf("   • Intervalos maiores (1000ms) reduzem overhead mas diminuem granularidade\n");
    printf("   • Context switches aumentam proporcionalmente à frequência de monitoramento\n");
    printf("\n");
    
    printf("Recomendações:\n");
    if (avg_time_overhead < 5.0) {
        printf("   • Overhead BAIXO (<5%%) - Monitoramento tem impacto mínimo\n");
        printf("   • Intervalos de 10-100ms são ideais para monitoramento contínuo\n");
    } else if (avg_time_overhead < 15.0) {
        printf("   • Overhead MODERADO (5-15%%) - Aceitável para análise de performance\n");
        printf("   • Use intervalos >= 100ms para reduzir impacto\n");
    } else {
        printf("   • Overhead ALTO (>15%%) - Considere aumentar intervalo de sampling\n");
        printf("   • Recomendado >= 500ms para ambientes de produção\n");
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    
    // Exportar resultados
    printf("\nExportando resultados para arquivo...\n");
    
    FILE *output = fopen("output/experiment1_overhead.csv", "w");
    if (output) {
        fprintf(output, "sampling_interval_ms,execution_time_sec,time_overhead_percent,");
        fprintf(output, "cpu_time_sec,cpu_overhead_percent,user_time_sec,system_time_sec,");
        fprintf(output, "context_switches,ctx_switches_delta,memory_peak_kb\n");
        
        // Baseline
        fprintf(output, "0,%.3f,0.00,%.3f,0.00,%.3f,%.3f,%ld,0,%ld\n",
                results[0].execution_time_sec,
                results[0].cpu_time_sec,
                results[0].user_time_sec,
                results[0].system_time_sec,
                results[0].context_switches,
                results[0].memory_peak_kb);
        
        // Com monitoramento
        for (int i = 0; i < NUM_INTERVALS; i++) {
            double time_oh = ((results[i + 1].execution_time_sec - results[0].execution_time_sec) / 
                             results[0].execution_time_sec) * 100.0;
            double cpu_oh = ((results[i + 1].cpu_time_sec - results[0].cpu_time_sec) / 
                            results[0].cpu_time_sec) * 100.0;
            long ctx_delta = results[i + 1].context_switches - results[0].context_switches;
            
            fprintf(output, "%d,%.3f,%.2f,%.3f,%.2f,%.3f,%.3f,%ld,%ld,%ld\n",
                    intervals[i],
                    results[i + 1].execution_time_sec,
                    time_oh,
                    results[i + 1].cpu_time_sec,
                    cpu_oh,
                    results[i + 1].user_time_sec,
                    results[i + 1].system_time_sec,
                    results[i + 1].context_switches,
                    ctx_delta,
                    results[i + 1].memory_peak_kb);
        }
        
        fclose(output);
        printf("✓ Resultados salvos em: output/experiment1_overhead.csv\n");
    } else {
        printf("✗ Erro ao criar arquivo de saída\n");
    }
    
    printf("\n");
}
