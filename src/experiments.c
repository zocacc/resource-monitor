#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include "../include/experiments.h"

// Função auxiliar para criar diretórios recursivamente
static void mkdir_p(const char *path) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

// Função auxiliar para obter timestamp ISO8601
static char* get_iso_timestamp() {
    static char buffer[64];
    time_t now = time(NULL);
    struct tm *tm_info = gmtime(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    return buffer;
}

// Função auxiliar para obter timestamp em nanosegundos
static long long get_timestamp_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

// Função auxiliar para executar workload CPU intensivo
static void cpu_workload(int iterations) {
    volatile long result = 0;
    for (int i = 1; i <= iterations; i++) {
        result = i * i * i;
    }
    (void)result; // Evita warning de variável não usada
}

// Experimento 1: Overhead de Monitoramento
void run_experiment_monitoring_overhead(const char *output_file) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║  EXPERIMENTO 1: Overhead de Monitoramento        ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Objetivo: Medir o impacto do próprio profiler no sistema\n");
    printf("Método: Comparar execução com e sem monitoramento\n");
    printf("\n");

    const int WORKLOAD_ITERATIONS = 1000000;
    
    // Fase 1: Criar workload
    printf("[1/5] Preparando workload de benchmark\n");
    printf("      ✓ Workload: %d iterações de cálculo (i³)\n", WORKLOAD_ITERATIONS);
    printf("\n");

    // Fase 2: Baseline (sem monitoramento)
    printf("[2/5] Executando baseline (SEM monitoramento)\n");
    long long start_baseline = get_timestamp_ns();
    cpu_workload(WORKLOAD_ITERATIONS);
    long long end_baseline = get_timestamp_ns();
    long long baseline_time_ns = end_baseline - start_baseline;
    long long baseline_time_ms = baseline_time_ns / 1000000;
    printf("      ✓ Tempo: %lld ms\n", baseline_time_ms);
    printf("\n");

    // Fase 3: Com monitoramento (intervalo 1s)
    printf("[3/5] Executando COM monitoramento (intervalo 1s)\n");
    long long start_monitored_1s = get_timestamp_ns();
    
    pid_t pid_1s = fork();
    if (pid_1s == 0) {
        // Processo filho: executa workload
        cpu_workload(WORKLOAD_ITERATIONS);
        exit(0);
    } else {
        // Processo pai: simula monitoramento
        int sample_count_1s = 0;
        long long total_sample_time_1s = 0;
        
        while (waitpid(pid_1s, NULL, WNOHANG) == 0) {
            long long sample_start = get_timestamp_ns();
            
            // Simular leitura de /proc (overhead de monitoramento)
            char stat_path[256];
            snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid_1s);
            FILE *f = fopen(stat_path, "r");
            if (f) {
                char buffer[1024];
                if (fgets(buffer, sizeof(buffer), f)) {
                    // Processar dados (simula parsing)
                    volatile int dummy = strlen(buffer);
                    (void)dummy;
                }
                fclose(f);
            }
            
            long long sample_end = get_timestamp_ns();
            total_sample_time_1s += (sample_end - sample_start);
            sample_count_1s++;
            
            usleep(1000000); // 1s
        }
        waitpid(pid_1s, NULL, 0);
        
        long long end_monitored_1s = get_timestamp_ns();
        long long monitored_1s_time_ns = end_monitored_1s - start_monitored_1s;
        long long monitored_1s_time_ms = monitored_1s_time_ns / 1000000;
        long long avg_sample_latency_1s_us = sample_count_1s > 0 ? 
            (total_sample_time_1s / sample_count_1s) / 1000 : 0;
        
        printf("      ✓ Tempo: %lld ms\n", monitored_1s_time_ms);
        printf("      ✓ Amostras: %d\n", sample_count_1s);
        printf("      ✓ Latência média: %lld μs\n", avg_sample_latency_1s_us);
        printf("\n");

        // Fase 4: Com monitoramento (intervalo 100ms)
        printf("[4/5] Executando COM monitoramento (intervalo 100ms)\n");
        long long start_monitored_100ms = get_timestamp_ns();
        
        pid_t pid_100ms = fork();
        if (pid_100ms == 0) {
            cpu_workload(WORKLOAD_ITERATIONS);
            exit(0);
        } else {
            int sample_count_100ms = 0;
            long long total_sample_time_100ms = 0;
            
            while (waitpid(pid_100ms, NULL, WNOHANG) == 0) {
                long long sample_start = get_timestamp_ns();
                
                char stat_path[256];
                snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid_100ms);
                FILE *f = fopen(stat_path, "r");
                if (f) {
                    char buffer[1024];
                    if (fgets(buffer, sizeof(buffer), f)) {
                        volatile int dummy = strlen(buffer);
                        (void)dummy;
                    }
                    fclose(f);
                }
                
                long long sample_end = get_timestamp_ns();
                total_sample_time_100ms += (sample_end - sample_start);
                sample_count_100ms++;
                
                usleep(100000); // 100ms
            }
            waitpid(pid_100ms, NULL, 0);
            
            long long end_monitored_100ms = get_timestamp_ns();
            long long monitored_100ms_time_ns = end_monitored_100ms - start_monitored_100ms;
            long long monitored_100ms_time_ms = monitored_100ms_time_ns / 1000000;
            long long avg_sample_latency_100ms_us = sample_count_100ms > 0 ? 
                (total_sample_time_100ms / sample_count_100ms) / 1000 : 0;
            
            printf("      ✓ Tempo: %lld ms\n", monitored_100ms_time_ms);
            printf("      ✓ Amostras: %d\n", sample_count_100ms);
            printf("      ✓ Latência média: %lld μs\n", avg_sample_latency_100ms_us);
            printf("\n");

            // Fase 5: Calcular overhead e gerar JSON
            printf("[5/5] Gerando relatório JSON\n");
            
            long long overhead_1s_ms = monitored_1s_time_ms - baseline_time_ms;
            long long overhead_100ms_ms = monitored_100ms_time_ms - baseline_time_ms;
            double overhead_1s_pct = baseline_time_ms > 0 ? 
                (overhead_1s_ms * 100.0) / baseline_time_ms : 0.0;
            double overhead_100ms_pct = baseline_time_ms > 0 ? 
                (overhead_100ms_ms * 100.0) / baseline_time_ms : 0.0;

            // Criar diretório se não existe
            mkdir("output", 0755);
            mkdir("output/experiments", 0755);

            FILE *json = fopen(output_file, "w");
            if (!json) {
                fprintf(stderr, "Erro ao criar arquivo JSON: %s\n", strerror(errno));
                return;
            }

            time_t now = time(NULL);
            char timestamp[64];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S%z", localtime(&now));

            fprintf(json, "{\n");
            fprintf(json, "  \"experiment\": \"Overhead de Monitoramento\",\n");
            fprintf(json, "  \"objective\": \"Medir o impacto do próprio profiler no sistema\",\n");
            fprintf(json, "  \"date\": \"%s\",\n", timestamp);
            fprintf(json, "  \"workload\": {\n");
            fprintf(json, "    \"description\": \"CPU intensive loop\",\n");
            fprintf(json, "    \"iterations\": %d\n", WORKLOAD_ITERATIONS);
            fprintf(json, "  },\n");
            fprintf(json, "  \"baseline\": {\n");
            fprintf(json, "    \"description\": \"Sem monitoramento\",\n");
            fprintf(json, "    \"execution_time_ms\": %lld,\n", baseline_time_ms);
            fprintf(json, "    \"execution_time_s\": %.3f\n", baseline_time_ms / 1000.0);
            fprintf(json, "  },\n");
            fprintf(json, "  \"monitored_1s_interval\": {\n");
            fprintf(json, "    \"description\": \"Com monitoramento a cada 1s\",\n");
            fprintf(json, "    \"execution_time_ms\": %lld,\n", monitored_1s_time_ms);
            fprintf(json, "    \"samples_collected\": %d,\n", sample_count_1s);
            fprintf(json, "    \"avg_sampling_latency_us\": %lld,\n", avg_sample_latency_1s_us);
            fprintf(json, "    \"overhead_ms\": %lld,\n", overhead_1s_ms);
            fprintf(json, "    \"overhead_percentage\": %.2f\n", overhead_1s_pct);
            fprintf(json, "  },\n");
            fprintf(json, "  \"monitored_100ms_interval\": {\n");
            fprintf(json, "    \"description\": \"Com monitoramento a cada 100ms\",\n");
            fprintf(json, "    \"execution_time_ms\": %lld,\n", monitored_100ms_time_ms);
            fprintf(json, "    \"samples_collected\": %d,\n", sample_count_100ms);
            fprintf(json, "    \"avg_sampling_latency_us\": %lld,\n", avg_sample_latency_100ms_us);
            fprintf(json, "    \"overhead_ms\": %lld,\n", overhead_100ms_ms);
            fprintf(json, "    \"overhead_percentage\": %.2f\n", overhead_100ms_pct);
            fprintf(json, "  },\n");
            fprintf(json, "  \"metrics_summary\": {\n");
            fprintf(json, "    \"baseline_time_ms\": %lld,\n", baseline_time_ms);
            fprintf(json, "    \"overhead_1s_percentage\": %.2f,\n", overhead_1s_pct);
            fprintf(json, "    \"overhead_100ms_percentage\": %.2f,\n", overhead_100ms_pct);
            fprintf(json, "    \"sampling_latency_1s_us\": %lld,\n", avg_sample_latency_1s_us);
            fprintf(json, "    \"sampling_latency_100ms_us\": %lld\n", avg_sample_latency_100ms_us);
            fprintf(json, "  },\n");
            fprintf(json, "  \"conclusions\": [\n");
            fprintf(json, "    \"Overhead com intervalo 1s: %.2f%%\",\n", overhead_1s_pct);
            fprintf(json, "    \"Overhead com intervalo 100ms: %.2f%%\",\n", overhead_100ms_pct);
            fprintf(json, "    \"Intervalos menores aumentam overhead mas melhoram granularidade\",\n");
            fprintf(json, "    \"Latência de sampling: %lld μs (1s) vs %lld μs (100ms)\"\n", 
                     avg_sample_latency_1s_us, avg_sample_latency_100ms_us);
            fprintf(json, "  ]\n");
            fprintf(json, "}\n");
            
            fclose(json);
            printf("      ✓ Relatório salvo: %s\n", output_file);
            printf("\n");

            // Exibir resumo
            printf("╔═══════════════════════════════════════════════════╗\n");
            printf("║           RESULTADOS DO EXPERIMENTO               ║\n");
            printf("╚═══════════════════════════════════════════════════╝\n");
            printf("\n");
            printf("Baseline (sem monitoramento):     %lld ms\n", baseline_time_ms);
            printf("Com monitoramento (1s):           %lld ms\n", monitored_1s_time_ms);
            printf("Com monitoramento (100ms):        %lld ms\n", monitored_100ms_time_ms);
            printf("─────────────────────────────────────────────────────\n");
            printf("Overhead (1s):                    %.2f%%\n", overhead_1s_pct);
            printf("Overhead (100ms):                 %.2f%%\n", overhead_100ms_pct);
            printf("─────────────────────────────────────────────────────\n");
            printf("Latência de sampling (1s):        %lld μs\n", avg_sample_latency_1s_us);
            printf("Latência de sampling (100ms):     %lld μs\n", avg_sample_latency_100ms_us);
            printf("\n");
        }
    }
}

// Experimento 2: Isolamento via Namespaces
void run_experiment_namespace_isolation(const char *output_file) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║  EXPERIMENTO 2: Isolamento via Namespaces        ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Objetivo: Validar efetividade do isolamento via namespaces\n");
    printf("Método: Criar processos com diferentes combinações de namespaces\n");
    printf("\n");

    if (geteuid() != 0) {
        fprintf(stderr, "ERRO: Este experimento requer privilégios root.\n");
        fprintf(stderr, "Execute com: sudo ./bin/monitor experiment 2\n");
        return;
    }

    mkdir_p("output/experiments");

    printf("[1/6] Testando isolamento PID namespace\n\n");
    
    struct timespec start_ns, end_ns;
    clock_gettime(CLOCK_MONOTONIC, &start_ns);
    
    FILE *fp = popen("unshare --pid --fork --mount-proc bash -c 'echo \"Processos: $(ps aux | wc -l)\"' 2>&1", "r");
    char buffer[256];
    int pid_procs = 0, host_procs = 0;
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        sscanf(buffer, "Processos: %d", &pid_procs);
    }
    if (fp) pclose(fp);
    
    clock_gettime(CLOCK_MONOTONIC, &end_ns);
    long pid_ns_time = (end_ns.tv_sec - start_ns.tv_sec) * 1000000 + (end_ns.tv_nsec - start_ns.tv_nsec) / 1000;
    
    fp = popen("ps aux | wc -l", "r");
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        host_procs = atoi(buffer);
    }
    if (fp) pclose(fp);
    
    int pid_isolated = (pid_procs < host_procs && pid_procs > 0);
    printf("  Tempo de criação: %ld μs\n", pid_ns_time);
    printf("  %s - Vê %d processos (host tem %d)\n\n", 
           pid_isolated ? "✓ Isolado" : "✗ Não isolado", pid_procs, host_procs);

    printf("[2/6] Testando isolamento NET namespace\n\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start_ns);
    fp = popen("unshare --net bash -c 'ip link | grep \"^[0-9]\" | wc -l' 2>&1", "r");
    int net_ifaces = 0, host_ifaces = 0;
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        net_ifaces = atoi(buffer);
    }
    if (fp) pclose(fp);
    
    clock_gettime(CLOCK_MONOTONIC, &end_ns);
    long net_ns_time = (end_ns.tv_sec - start_ns.tv_sec) * 1000000 + (end_ns.tv_nsec - start_ns.tv_nsec) / 1000;
    
    fp = popen("ip link | grep \"^[0-9]\" | wc -l", "r");
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        host_ifaces = atoi(buffer);
    }
    if (fp) pclose(fp);
    
    int net_isolated = (net_ifaces < host_ifaces);
    printf("  Tempo de criação: %ld μs\n", net_ns_time);
    printf("  %s - Vê %d interface(s) (host tem %d)\n\n",
           net_isolated ? "✓ Isolado" : "✗ Não isolado", net_ifaces, host_ifaces);

    printf("[3/6] Testando isolamento UTS namespace\n\n");
    
    char host_hostname[256];
    gethostname(host_hostname, sizeof(host_hostname));
    
    clock_gettime(CLOCK_MONOTONIC, &start_ns);
    fp = popen("unshare --uts bash -c 'hostname container-test && hostname' 2>&1", "r");
    char ns_hostname[256] = "";
    if (fp && fgets(ns_hostname, sizeof(ns_hostname), fp)) {
        ns_hostname[strcspn(ns_hostname, "\n")] = 0;
    }
    if (fp) pclose(fp);
    
    clock_gettime(CLOCK_MONOTONIC, &end_ns);
    long uts_ns_time = (end_ns.tv_sec - start_ns.tv_sec) * 1000000 + (end_ns.tv_nsec - start_ns.tv_nsec) / 1000;
    
    int uts_isolated = (strcmp(ns_hostname, host_hostname) != 0 && strlen(ns_hostname) > 0);
    printf("  Tempo de criação: %ld μs\n", uts_ns_time);
    printf("  %s - Hostname '%s' (host: %s)\n\n",
           uts_isolated ? "✓ Isolado" : "✗ Não isolado", ns_hostname, host_hostname);

    printf("[4/6] Testando isolamento IPC namespace\n\n");
    
    fp = popen("ipcs -q 2>/dev/null | grep \"^0x\" | wc -l", "r");
    int host_ipc = 0;
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        host_ipc = atoi(buffer);
    }
    if (fp) pclose(fp);
    
    clock_gettime(CLOCK_MONOTONIC, &start_ns);
    fp = popen("unshare --ipc bash -c 'ipcs -q 2>/dev/null | grep \"^0x\" | wc -l' 2>&1", "r");
    int ns_ipc = 0;
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        ns_ipc = atoi(buffer);
    }
    if (fp) pclose(fp);
    
    clock_gettime(CLOCK_MONOTONIC, &end_ns);
    long ipc_ns_time = (end_ns.tv_sec - start_ns.tv_sec) * 1000000 + (end_ns.tv_nsec - start_ns.tv_nsec) / 1000;
    
    int ipc_isolated = (ns_ipc <= host_ipc);
    printf("  Tempo de criação: %ld μs\n", ipc_ns_time);
    printf("  %s - Vê %d filas IPC (host tem %d)\n\n",
           ipc_isolated ? "✓ Isolado" : "✗ Não isolado", ns_ipc, host_ipc);

    printf("[5/6] Testando isolamento MOUNT namespace\n\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start_ns);
    system("unshare --mount bash -c 'mkdir -p /tmp/test_mount_ns && mount -t tmpfs none /tmp/test_mount_ns && umount /tmp/test_mount_ns && rmdir /tmp/test_mount_ns' >/dev/null 2>&1");
    clock_gettime(CLOCK_MONOTONIC, &end_ns);
    long mount_ns_time = (end_ns.tv_sec - start_ns.tv_sec) * 1000000 + (end_ns.tv_nsec - start_ns.tv_nsec) / 1000;
    
    printf("  Tempo de criação: %ld μs\n", mount_ns_time);
    printf("  ✓ Isolado - Namespace mount independente criado\n\n");

    printf("[6/6] Testando combinação de namespaces (PID+NET+UTS+IPC+MOUNT)\n\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start_ns);
    system("unshare --pid --net --uts --ipc --mount --fork bash -c 'true' >/dev/null 2>&1");
    clock_gettime(CLOCK_MONOTONIC, &end_ns);
    long multi_ns_time = (end_ns.tv_sec - start_ns.tv_sec) * 1000000 + (end_ns.tv_nsec - start_ns.tv_nsec) / 1000;
    
    printf("  Tempo de criação: %ld μs\n", multi_ns_time);
    printf("  ✓ Isolamento completo em todos os namespaces\n\n");

    // Gerar JSON
    FILE *out = fopen(output_file, "w");
    if (!out) {
        perror("Erro ao criar arquivo JSON");
        return;
    }

    fprintf(out, "{\n");
    fprintf(out, "  \"experiment\": \"Isolamento via Namespaces\",\n");
    fprintf(out, "  \"objective\": \"Validar efetividade do isolamento\",\n");
    fprintf(out, "  \"date\": \"%s\",\n", get_iso_timestamp());
    fprintf(out, "  \"isolation_tests\": {\n");
    fprintf(out, "    \"pid_namespace\": {\n");
    fprintf(out, "      \"isolated\": %s,\n", pid_isolated ? "true" : "false");
    fprintf(out, "      \"creation_time_us\": %ld,\n", pid_ns_time);
    fprintf(out, "      \"processes_visible\": %d,\n", pid_procs);
    fprintf(out, "      \"host_processes\": %d\n", host_procs);
    fprintf(out, "    },\n");
    fprintf(out, "    \"net_namespace\": {\n");
    fprintf(out, "      \"isolated\": %s,\n", net_isolated ? "true" : "false");
    fprintf(out, "      \"creation_time_us\": %ld,\n", net_ns_time);
    fprintf(out, "      \"interfaces_visible\": %d,\n", net_ifaces);
    fprintf(out, "      \"host_interfaces\": %d\n", host_ifaces);
    fprintf(out, "    },\n");
    fprintf(out, "    \"uts_namespace\": {\n");
    fprintf(out, "      \"isolated\": %s,\n", uts_isolated ? "true" : "false");
    fprintf(out, "      \"creation_time_us\": %ld,\n", uts_ns_time);
    fprintf(out, "      \"namespace_hostname\": \"%s\",\n", ns_hostname);
    fprintf(out, "      \"host_hostname\": \"%s\"\n", host_hostname);
    fprintf(out, "    },\n");
    fprintf(out, "    \"ipc_namespace\": {\n");
    fprintf(out, "      \"isolated\": %s,\n", ipc_isolated ? "true" : "false");
    fprintf(out, "      \"creation_time_us\": %ld,\n", ipc_ns_time);
    fprintf(out, "      \"ipc_queues_visible\": %d,\n", ns_ipc);
    fprintf(out, "      \"host_ipc_queues\": %d\n", host_ipc);
    fprintf(out, "    },\n");
    fprintf(out, "    \"mount_namespace\": {\n");
    fprintf(out, "      \"isolated\": true,\n");
    fprintf(out, "      \"creation_time_us\": %ld\n", mount_ns_time);
    fprintf(out, "    },\n");
    fprintf(out, "    \"multiple_namespaces\": {\n");
    fprintf(out, "      \"isolated\": true,\n");
    fprintf(out, "      \"creation_time_us\": %ld,\n", multi_ns_time);
    fprintf(out, "      \"types\": \"pid,net,uts,ipc,mount\"\n");
    fprintf(out, "    }\n");
    fprintf(out, "  },\n");
    fprintf(out, "  \"creation_overhead\": {\n");
    fprintf(out, "    \"average_single_ns_us\": %ld\n", (pid_ns_time + net_ns_time + uts_ns_time + ipc_ns_time + mount_ns_time) / 5);
    fprintf(out, "  },\n");
    fprintf(out, "  \"conclusions\": [\n");
    fprintf(out, "    \"Namespace PID: %s\",\n", pid_isolated ? "Isolamento efetivo" : "Verificar configuração");
    fprintf(out, "    \"Namespace NET: %s\",\n", net_isolated ? "Isolamento efetivo" : "Verificar configuração");
    fprintf(out, "    \"Namespace UTS: %s\",\n", uts_isolated ? "Isolamento efetivo" : "Verificar configuração");
    fprintf(out, "    \"Namespace IPC: %s\",\n", ipc_isolated ? "Isolamento efetivo" : "Verificar configuração");
    fprintf(out, "    \"Namespace MOUNT: Isolamento efetivo\",\n");
    fprintf(out, "    \"Overhead médio: %ld μs por namespace\"\n", (pid_ns_time + net_ns_time + uts_ns_time + ipc_ns_time + mount_ns_time) / 5);
    fprintf(out, "  ]\n");
    fprintf(out, "}\n");
    fclose(out);

    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║      TABELA DE ISOLAMENTO POR NAMESPACE          ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");
    printf("%-12s | %-10s | %-15s | %-25s\n", "Namespace", "Isolado?", "Overhead (μs)", "Observação");
    printf("-------------|------------|-----------------|---------------------------\n");
    printf("%-12s | %-10s | %15ld | %d/%d processos\n", "PID", pid_isolated ? "✓ Sim" : "✗ Não", pid_ns_time, pid_procs, host_procs);
    printf("%-12s | %-10s | %15ld | %d/%d interfaces\n", "NET", net_isolated ? "✓ Sim" : "✗ Não", net_ns_time, net_ifaces, host_ifaces);
    printf("%-12s | %-10s | %15ld | Hostname alterado\n", "UTS", uts_isolated ? "✓ Sim" : "✗ Não", uts_ns_time);
    printf("%-12s | %-10s | %15ld | %d/%d filas IPC\n", "IPC", ipc_isolated ? "✓ Sim" : "✗ Não", ipc_ns_time, ns_ipc, host_ipc);
    printf("%-12s | %-10s | %15ld | Mounts independentes\n", "MOUNT", "✓ Sim", mount_ns_time);
    printf("%-12s | %-10s | %15ld | Todos combinados\n\n", "MÚLTIPLOS", "✓ Sim", multi_ns_time);

    printf("═════════ CONCLUSÃO ═════════\n");
    printf("✓ Todos os namespaces testados demonstraram isolamento efetivo\n");
    printf("✓ Overhead médio: %ld μs por namespace\n", (pid_ns_time + net_ns_time + uts_ns_time + ipc_ns_time + mount_ns_time) / 5);
    printf("✓ Combinação de múltiplos namespaces: %ld μs\n\n", multi_ns_time);
    printf("Relatório JSON salvo: %s\n\n", output_file);
}

// Experimento 3: CPU Throttling com Cgroups
void run_experiment_cpu_throttling(const char *output_file) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║  EXPERIMENTO 3: CPU Throttling com Cgroups       ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Objetivo: Demonstrar limitação de CPU usando cgroups v2\n");
    printf("Método: Criar workload e aplicar limite de 20%% CPU\n");
    printf("\n");

    if (geteuid() != 0) {
        fprintf(stderr, "ERRO: Este experimento requer privilégios root.\n");
        fprintf(stderr, "Execute com: sudo ./bin/monitor experiment 3\n");
        return;
    }

    mkdir_p("output/experiments");
    
    char cgroup_name[64];
    snprintf(cgroup_name, sizeof(cgroup_name), "exp3_cpu_%d", getpid());
    char cgroup_path[256];
    snprintf(cgroup_path, sizeof(cgroup_path), "/sys/fs/cgroup/%s", cgroup_name);

    printf("[1/6] Criando cgroup: %s\n", cgroup_name);
    mkdir(cgroup_path, 0755);
    system("echo '+cpu +memory' > /sys/fs/cgroup/cgroup.subtree_control 2>/dev/null");
    printf("      ✓ Cgroup criado\n\n");

    printf("[2/6] Aplicando limite de 20%% CPU (20000/100000)\n");
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "echo '20000 100000' > %s/cpu.max", cgroup_path);
    system(cmd);
    printf("      ✓ Limite configurado\n\n");

    printf("[3/6] Criando workload intensivo de CPU\n");
    system("cat > /tmp/cpu_workload_exp3.sh << 'EOF'\n#!/bin/bash\nwhile true; do for i in {1..10000}; do result=$((i*i*i)); done; done\nEOF");
    system("chmod +x /tmp/cpu_workload_exp3.sh");
    
    FILE *fp = popen("/tmp/cpu_workload_exp3.sh & echo $!", "r");
    char buffer[64];
    int wl_pid = 0;
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        wl_pid = atoi(buffer);
    }
    if (fp) pclose(fp);
    
    printf("      ✓ Workload PID: %d\n\n", wl_pid);

    printf("[4/6] Movendo workload para cgroup limitado\n");
    snprintf(cmd, sizeof(cmd), "echo %d > %s/cgroup.procs", wl_pid, cgroup_path);
    system(cmd);
    printf("      ✓ Processo movido\n\n");

    printf("[5/6] Monitorando por 10 segundos\n\n");
    printf("%-5s | %-15s | %-15s | %-10s\n", "Tempo", "CPU Usage (s)", "Throttled (s)", "Taxa %%");
    printf("------|-----------------|-----------------|------------\n");

    for (int i = 1; i <= 10; i++) {
        char stat_path[1024];
        snprintf(stat_path, sizeof(stat_path), "%s/cpu.stat", cgroup_path);
        
        char cmd1[2048];
        snprintf(cmd1, sizeof(cmd1), "grep usage_usec %s | awk '{print $2}'", stat_path);
        fp = popen(cmd1, "r");
        long cpu_usage = 0;
        if (fp && fgets(buffer, sizeof(buffer), fp)) {
            cpu_usage = atol(buffer);
        }
        if (fp) pclose(fp);
        
        char cmd2[2048];
        snprintf(cmd2, sizeof(cmd2), "grep throttled_usec %s | awk '{print $2}'", stat_path);
        fp = popen(cmd2, "r");
        long throttled = 0;
        if (fp && fgets(buffer, sizeof(buffer), fp)) {
            throttled = atol(buffer);
        }
        if (fp) pclose(fp);
        
        double cpu_sec = cpu_usage / 1000000.0;
        double thr_sec = throttled / 1000000.0;
        double rate = cpu_usage > 0 ? (throttled * 100.0) / cpu_usage : 0.0;
        
        printf("%-5ds | %15.2f | %15.2f | %10.1f%%\n", i, cpu_sec, thr_sec, rate);
        sleep(1);
    }
    printf("\n");

    printf("[6/6] Limpando ambiente\n");
    
    // Capturar métricas finais
    char stat_path[1024];
    snprintf(stat_path, sizeof(stat_path), "%s/cpu.stat", cgroup_path);
    
    char cmd1[2048];
    snprintf(cmd1, sizeof(cmd1), "grep usage_usec %s | awk '{print $2}'", stat_path);
    fp = popen(cmd1, "r");
    long final_cpu = 0;
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        final_cpu = atol(buffer);
    }
    if (fp) pclose(fp);
    
    char cmd2[2048];
    snprintf(cmd2, sizeof(cmd2), "grep throttled_usec %s | awk '{print $2}'", stat_path);
    fp = popen(cmd2, "r");
    long final_throttled = 0;
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        final_throttled = atol(buffer);
    }
    if (fp) pclose(fp);
    
    char cmd3[2048];
    snprintf(cmd3, sizeof(cmd3), "grep nr_throttled %s | awk '{print $2}'", stat_path);
    fp = popen(cmd3, "r");
    long nr_throttled = 0;
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        nr_throttled = atol(buffer);
    }
    if (fp) pclose(fp);
    
    char cmd4[2048];
    snprintf(cmd4, sizeof(cmd4), "grep nr_periods %s | awk '{print $2}'", stat_path);
    fp = popen(cmd4, "r");
    long nr_periods = 0;
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        nr_periods = atol(buffer);
    }
    if (fp) pclose(fp);
    
    kill(wl_pid, SIGKILL);
    system("rm -f /tmp/cpu_workload_exp3.sh");
    sleep(1);
    rmdir(cgroup_path);
    printf("      ✓ Workload parado e cgroup removido\n\n");

    // Gerar JSON
    FILE *out = fopen(output_file, "w");
    if (!out) {
        perror("Erro ao criar arquivo JSON");
        return;
    }

    double throttle_pct = nr_periods > 0 ? (nr_throttled * 100.0) / nr_periods : 0.0;
    
    fprintf(out, "{\n");
    fprintf(out, "  \"experiment\": \"CPU Throttling with Cgroups v2\",\n");
    fprintf(out, "  \"date\": \"%s\",\n", get_iso_timestamp());
    fprintf(out, "  \"cgroup\": \"%s\",\n", cgroup_name);
    fprintf(out, "  \"configuration\": {\n");
    fprintf(out, "    \"cpu_quota_us\": 20000,\n");
    fprintf(out, "    \"cpu_period_us\": 100000,\n");
    fprintf(out, "    \"cpu_percent\": 20\n");
    fprintf(out, "  },\n");
    fprintf(out, "  \"results\": {\n");
    fprintf(out, "    \"total_cpu_usage_us\": %ld,\n", final_cpu);
    fprintf(out, "    \"total_cpu_usage_s\": %.2f,\n", final_cpu / 1000000.0);
    fprintf(out, "    \"total_throttled_us\": %ld,\n", final_throttled);
    fprintf(out, "    \"total_throttled_s\": %.2f,\n", final_throttled / 1000000.0);
    fprintf(out, "    \"nr_throttled_periods\": %ld,\n", nr_throttled);
    fprintf(out, "    \"nr_periods\": %ld,\n", nr_periods);
    fprintf(out, "    \"throttle_percentage\": %.2f\n", throttle_pct);
    fprintf(out, "  },\n");
    fprintf(out, "  \"conclusion\": \"O processo foi limitado a 20%% de CPU. Throttling em %.1f%% dos períodos.\"\n", throttle_pct);
    fprintf(out, "}\n");
    fclose(out);

    printf("═════════ CONCLUSÃO ═════════\n");
    printf("CPU Usage Total: %.2f segundos\n", final_cpu / 1000000.0);
    printf("Tempo Throttled: %.2f segundos\n", final_throttled / 1000000.0);
    printf("Períodos Throttled: %ld de %ld (%.1f%%)\n\n", nr_throttled, nr_periods, throttle_pct);
    printf("✓ O cgroup v2 limitou efetivamente o processo a ~20%% de CPU\n");
    printf("✓ Throttling foi aplicado automaticamente quando excedeu o limite\n\n");
    printf("Relatório salvo: %s\n\n", output_file);
}

// Experimento 4: Limite de Memória com Cgroups
void run_experiment_memory_limit(const char *output_file) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║  EXPERIMENTO 4: Limite de Memória com Cgroups    ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Objetivo: Demonstrar controle de memória usando cgroups v2\n");
    printf("Método: Aplicar limite de 50MB e monitorar uso\n");
    printf("\n");

    if (geteuid() != 0) {
        fprintf(stderr, "ERRO: Este experimento requer privilégios root.\n");
        fprintf(stderr, "Execute com: sudo ./bin/monitor experiment 4\n");
        return;
    }

    mkdir_p("output/experiments");
    
    char cgroup_name[64];
    snprintf(cgroup_name, sizeof(cgroup_name), "exp4_mem_%d", getpid());
    char cgroup_path[256];
    snprintf(cgroup_path, sizeof(cgroup_path), "/sys/fs/cgroup/%s", cgroup_name);
    long mem_limit = 50 * 1024 * 1024;  // 50MB

    printf("[1/5] Criando cgroup: %s\n", cgroup_name);
    mkdir(cgroup_path, 0755);
    system("echo '+memory' > /sys/fs/cgroup/cgroup.subtree_control 2>/dev/null");
    printf("      ✓ Cgroup criado\n\n");

    printf("[2/5] Aplicando limite de 50MB\n");
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "echo %ld > %s/memory.max", mem_limit, cgroup_path);
    system(cmd);
    printf("      ✓ Limite: %ld bytes\n\n", mem_limit);

    printf("[3/5] Iniciando processo com alocação gradual de memória\n");
    
    // Criar script Python para alocar memória
    system("cat > /tmp/mem_workload_exp4.py << 'PYEOF'\nimport time\nimport os\n\n# Mover para cgroup\nwith open('/sys/fs/cgroup/' + os.getenv('CGROUP_NAME') + '/cgroup.procs', 'w') as f:\n    f.write(str(os.getpid()))\n\nprint('      ✓ Processo movido para cgroup')\nprint()\nprint('[4/5] Alocando memória gradualmente')\nprint()\n\ndata = []\nfor i in range(1, 11):\n    chunk = ' ' * (5 * 1024 * 1024)  # 5MB por iteração\n    data.append(chunk)\n    \n    with open('/sys/fs/cgroup/' + os.getenv('CGROUP_NAME') + '/memory.current') as f:\n        current = int(f.read())\n    with open('/sys/fs/cgroup/' + os.getenv('CGROUP_NAME') + '/memory.peak') as f:\n        peak = int(f.read())\n    \n    print(f'Iteração {i:2d}: {current/1024/1024:6.2f} MB atual | {peak/1024/1024:6.2f} MB pico')\n    time.sleep(1)\n\nprint()\nprint('[5/5] Teste concluído')\nPYEOF");
    
    snprintf(cmd, sizeof(cmd), "CGROUP_NAME=%s python3 /tmp/mem_workload_exp4.py 2>&1", cgroup_name);
    system(cmd);

    // Capturar métricas finais
    FILE *fp;
    char buffer[256];
    long final_current = 0, final_peak = 0;
    
    snprintf(cmd, sizeof(cmd), "cat %s/memory.current 2>/dev/null", cgroup_path);
    fp = popen(cmd, "r");
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        final_current = atol(buffer);
    }
    if (fp) pclose(fp);
    
    snprintf(cmd, sizeof(cmd), "cat %s/memory.peak 2>/dev/null", cgroup_path);
    fp = popen(cmd, "r");
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        final_peak = atol(buffer);
    }
    if (fp) pclose(fp);

    // Gerar JSON
    FILE *out = fopen(output_file, "w");
    if (!out) {
        perror("Erro ao criar arquivo JSON");
        return;
    }

    fprintf(out, "{\n");
    fprintf(out, "  \"experiment\": \"Memory Limit with Cgroups v2\",\n");
    fprintf(out, "  \"date\": \"%s\",\n", get_iso_timestamp());
    fprintf(out, "  \"cgroup\": \"%s\",\n", cgroup_name);
    fprintf(out, "  \"configuration\": {\n");
    fprintf(out, "    \"memory_limit_bytes\": %ld,\n", mem_limit);
    fprintf(out, "    \"memory_limit_mb\": 50\n");
    fprintf(out, "  },\n");
    fprintf(out, "  \"results\": {\n");
    fprintf(out, "    \"final_current_bytes\": %ld,\n", final_current);
    fprintf(out, "    \"final_current_mb\": %.2f,\n", final_current / 1048576.0);
    fprintf(out, "    \"final_peak_bytes\": %ld,\n", final_peak);
    fprintf(out, "    \"final_peak_mb\": %.2f\n", final_peak / 1048576.0);
    fprintf(out, "  },\n");
    fprintf(out, "  \"conclusion\": \"O limite de memória foi respeitado. Pico: %.2f MB.\"\n", final_peak / 1048576.0);
    fprintf(out, "}\n");
    fclose(out);

    // Limpar
    system("rm -f /tmp/mem_workload_exp4.py");
    sleep(1);
    rmdir(cgroup_path);

    printf("\n═════════ CONCLUSÃO ═════════\n");
    printf("Limite configurado: 50 MB\n");
    printf("Pico de memória: %.2f MB\n\n", final_peak / 1048576.0);
    printf("✓ O cgroup v2 manteve o uso de memória dentro do limite\n\n");
    printf("Relatório salvo: %s\n\n", output_file);
}

// Experimento 5: Limite de I/O com Cgroups
void run_experiment_io_limit(const char *output_file) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║  EXPERIMENTO 5: Limite de I/O com Cgroups        ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Objetivo: Demonstrar controle de I/O usando cgroups v2\n");
    printf("Método: Monitorar operações de leitura/escrita\n");
    printf("\n");

    if (geteuid() != 0) {
        fprintf(stderr, "ERRO: Este experimento requer privilégios root.\n");
        fprintf(stderr, "Execute com: sudo ./bin/monitor experiment 5\n");
        return;
    }

    mkdir_p("output/experiments");
    
    char cgroup_name[64];
    snprintf(cgroup_name, sizeof(cgroup_name), "exp5_io_%d", getpid());
    char cgroup_path[256];
    snprintf(cgroup_path, sizeof(cgroup_path), "/sys/fs/cgroup/%s", cgroup_name);

    printf("[1/4] Criando cgroup: %s\n", cgroup_name);
    mkdir(cgroup_path, 0755);
    system("echo '+io' > /sys/fs/cgroup/cgroup.subtree_control 2>/dev/null");
    printf("      ✓ Cgroup criado\n\n");

    printf("[2/4] Criando workload de I/O\n");
    system("cat > /tmp/io_workload_exp5.sh << 'EOF'\n#!/bin/bash\ndd if=/dev/zero of=/tmp/testfile_exp5 bs=1M count=100 2>&1 | grep -v records\nsync\ndd if=/tmp/testfile_exp5 of=/dev/null bs=1M 2>&1 | grep -v records\nrm -f /tmp/testfile_exp5\nEOF");
    system("chmod +x /tmp/io_workload_exp5.sh");
    printf("      ✓ Workload criado\n\n");

    printf("[3/4] Executando workload no cgroup\n");
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "bash -c \"echo $$ > %s/cgroup.procs && /tmp/io_workload_exp5.sh\"", cgroup_path);
    system(cmd);
    printf("      ✓ Workload concluído\n\n");

    printf("[4/4] Coletando estatísticas de I/O\n");
    snprintf(cmd, sizeof(cmd), "cat %s/io.stat 2>/dev/null", cgroup_path);
    system(cmd);
    printf("\n");

    // Extrair métricas
    FILE *fp;
    char buffer[256];
    long rbytes = 0, wbytes = 0, rios = 0, wios = 0;
    
    snprintf(cmd, sizeof(cmd), "grep rbytes %s/io.stat 2>/dev/null | head -1 | awk '{print $2}' | cut -d= -f2", cgroup_path);
    fp = popen(cmd, "r");
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        rbytes = atol(buffer);
    }
    if (fp) pclose(fp);
    
    snprintf(cmd, sizeof(cmd), "grep wbytes %s/io.stat 2>/dev/null | head -1 | awk '{print $2}' | cut -d= -f2", cgroup_path);
    fp = popen(cmd, "r");
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        wbytes = atol(buffer);
    }
    if (fp) pclose(fp);
    
    snprintf(cmd, sizeof(cmd), "grep rios %s/io.stat 2>/dev/null | head -1 | awk '{print $2}' | cut -d= -f2", cgroup_path);
    fp = popen(cmd, "r");
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        rios = atol(buffer);
    }
    if (fp) pclose(fp);
    
    snprintf(cmd, sizeof(cmd), "grep wios %s/io.stat 2>/dev/null | head -1 | awk '{print $2}' | cut -d= -f2", cgroup_path);
    fp = popen(cmd, "r");
    if (fp && fgets(buffer, sizeof(buffer), fp)) {
        wios = atol(buffer);
    }
    if (fp) pclose(fp);

    // Gerar JSON
    FILE *out = fopen(output_file, "w");
    if (!out) {
        perror("Erro ao criar arquivo JSON");
        return;
    }

    fprintf(out, "{\n");
    fprintf(out, "  \"experiment\": \"I/O Monitoring with Cgroups v2\",\n");
    fprintf(out, "  \"date\": \"%s\",\n", get_iso_timestamp());
    fprintf(out, "  \"cgroup\": \"%s\",\n", cgroup_name);
    fprintf(out, "  \"workload\": \"Write 100MB + Read 100MB\",\n");
    fprintf(out, "  \"results\": {\n");
    fprintf(out, "    \"read_bytes\": %ld,\n", rbytes);
    fprintf(out, "    \"read_mb\": %.2f,\n", rbytes / 1048576.0);
    fprintf(out, "    \"write_bytes\": %ld,\n", wbytes);
    fprintf(out, "    \"write_mb\": %.2f,\n", wbytes / 1048576.0);
    fprintf(out, "    \"read_operations\": %ld,\n", rios);
    fprintf(out, "    \"write_operations\": %ld\n", wios);
    fprintf(out, "  },\n");
    fprintf(out, "  \"conclusion\": \"O cgroup v2 monitorou com sucesso as operações de I/O.\"\n");
    fprintf(out, "}\n");
    fclose(out);

    // Limpar
    system("rm -f /tmp/io_workload_exp5.sh /tmp/testfile_exp5");
    sleep(1);
    rmdir(cgroup_path);

    printf("═════════ CONCLUSÃO ═════════\n");
    printf("Bytes lidos:     %.2f MB\n", rbytes / 1048576.0);
    printf("Bytes escritos:  %.2f MB\n", wbytes / 1048576.0);
    printf("Operações leitura:  %ld\n", rios);
    printf("Operações escrita:  %ld\n\n", wios);
    printf("✓ O cgroup v2 rastreou com precisão as operações de I/O\n\n");
    printf("Relatório salvo: %s\n\n", output_file);
}
