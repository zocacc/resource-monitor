#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include "../include/utils.h"

#define MEMORY_LIMIT_MB 100
#define ALLOCATION_STEP_MB 10
#define MAX_ALLOCATIONS 30

// Função auxiliar para obter timestamp ISO8601
static char* get_iso_timestamp() {
    static char buffer[64];
    time_t now = time(NULL);
    struct tm *tm_info = gmtime(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    return buffer;
}

typedef struct {
    int step;
    long allocated_mb;
    long current_usage_mb;
    long peak_usage_mb;
    int allocation_success;
    const char *status_message;
} AllocationStep;

// Ler contador de falhas de memória
static long read_memory_failcnt(const char *cgroup_path) {
    char path[512];
    snprintf(path, sizeof(path), "%s/memory.events", cgroup_path);
    
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    
    long oom_count = 0, oom_kill_count = 0, max_count = 0, high_count = 0;
    char line[256];
    
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "oom ", 4) == 0) {
            sscanf(line, "oom %ld", &oom_count);
        } else if (strncmp(line, "oom_kill ", 9) == 0) {
            sscanf(line, "oom_kill %ld", &oom_kill_count);
        } else if (strncmp(line, "max ", 4) == 0) {
            sscanf(line, "max %ld", &max_count);
        } else if (strncmp(line, "high ", 5) == 0) {
            sscanf(line, "high %ld", &high_count);
        }
    }
    
    fclose(f);
    
    // Retornar soma de eventos de pressão de memória
    return oom_count + max_count;
}

// Ler uso atual de memória
static long read_memory_current(const char *cgroup_path) {
    char path[512];
    snprintf(path, sizeof(path), "%s/memory.current", cgroup_path);
    
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    
    long value = 0;
    if (fscanf(f, "%ld", &value) != 1) {
        value = 0;
    }
    
    fclose(f);
    return value;
}

// Ler pico de memória
static long read_memory_peak(const char *cgroup_path) {
    char path[512];
    snprintf(path, sizeof(path), "%s/memory.peak", cgroup_path);
    
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    
    long value = 0;
    if (fscanf(f, "%ld", &value) != 1) {
        value = 0;
    }
    
    fclose(f);
    return value;
}

void run_experiment_memory_limit_new() {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════════╗\n");
    printf("║           Experimento 4: Limitação de Memória                        ║\n");
    printf("╚═══════════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Objetivo: Testar comportamento ao atingir limite de memória\n\n");
    printf("Configuração:\n");
    printf("  • Limite de memória: %d MB\n", MEMORY_LIMIT_MB);
    printf("  • Passo de alocação: %d MB\n", ALLOCATION_STEP_MB);
    printf("  • Máximo de tentativas: %d iterações\n", MAX_ALLOCATIONS);
    printf("  • Observação: OOM killer, falhas de alocação\n\n");
    
    if (geteuid() != 0) {
        fprintf(stderr, "ERRO: Este experimento requer privilégios root.\n");
        fprintf(stderr, "Execute com: sudo ./bin/monitor experiment 4\n");
        return;
    }
    
    char cgroup_name[64];
    snprintf(cgroup_name, sizeof(cgroup_name), "exp4_mem_%d", getpid());
    char cgroup_path[1024];
    int path_len = snprintf(cgroup_path, sizeof(cgroup_path), "/sys/fs/cgroup/%s", cgroup_name);
    (void)path_len; // Evitar warning de variável não usada
    
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("FASE 1: Preparação do ambiente\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    
    // Habilitar controlador de memória
    FILE *f = fopen("/sys/fs/cgroup/cgroup.subtree_control", "w");
    if (f) {
        fprintf(f, "+memory\n");
        fclose(f);
        printf("✓ Controlador de memória habilitado\n");
    }
    
    // Criar cgroup
    if (mkdir(cgroup_path, 0755) != 0 && errno != EEXIST) {
        printf("✗ Erro ao criar cgroup: %s\n", strerror(errno));
        return;
    }
    printf("✓ Cgroup criado: %s\n", cgroup_name);
    
    // Configurar limite de memória
    char mem_max_path[2048];
    snprintf(mem_max_path, sizeof(mem_max_path), "%s/memory.max", cgroup_path);
    f = fopen(mem_max_path, "w");
    if (f) {
        fprintf(f, "%ld\n", (long)MEMORY_LIMIT_MB * 1024 * 1024);
        fclose(f);
        printf("✓ Limite configurado: %d MB\n", MEMORY_LIMIT_MB);
    } else {
        printf("✗ Erro ao configurar limite\n");
        rmdir(cgroup_path);
        return;
    }
    
    // Desabilitar swap para teste mais claro
    char swap_max_path[2048];
    snprintf(swap_max_path, sizeof(swap_max_path), "%s/memory.swap.max", cgroup_path);
    f = fopen(swap_max_path, "w");
    if (f) {
        fprintf(f, "0\n");
        fclose(f);
        printf("✓ Swap desabilitado para teste limpo\n");
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("FASE 2: Alocação incremental de memória\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    AllocationStep steps[MAX_ALLOCATIONS];
    int num_steps = 0;
    int oom_killed = 0;
    long max_allocated = 0;
    
    printf("%-6s | %-15s | %-15s | %-15s | %-10s\n",
           "Passo", "Tentativa (MB)", "Uso Atual (MB)", "Pico (MB)", "Status");
    printf("-------|-----------------|-----------------|-----------------|------------\n");
    fflush(stdout); // Garantir flush antes de fork
    
    for (int i = 0; i < MAX_ALLOCATIONS; i++) {
        long target_mb = (i + 1) * ALLOCATION_STEP_MB;
        
        // Fork para tentar alocação em processo separado
        pid_t pid = fork();
        
        if (pid == 0) {
            // Processo filho - tentar alocação
            
            // Adicionar ao cgroup
            char procs_path[2048];
            snprintf(procs_path, sizeof(procs_path), "%s/cgroup.procs", cgroup_path);
            FILE *pf = fopen(procs_path, "w");
            if (pf) {
                fprintf(pf, "%d\n", getpid());
                fclose(pf);
            }
            
            // Tentar alocar memória
            size_t size = (size_t)target_mb * 1024 * 1024;
            char *mem = (char *)malloc(size);
            
            if (mem) {
                // Tocar a memória para forçar alocação física
                for (size_t j = 0; j < size; j += 4096) {
                    mem[j] = 1;
                }
                
                // Manter alocado por um tempo
                sleep(1);
                
                // Sinalizar sucesso
                exit(0);
            } else {
                // Falha de alocação
                exit(1);
            }
            // Nunca chega aqui, mas garante que não continua loop
            _exit(1);
        } else if (pid > 0) {
            // Processo pai - monitorar
            sleep(2); // Esperar alocação e leitura de memória
            
            int status;
            pid_t result = waitpid(pid, &status, WNOHANG);
            
            long current_mb = read_memory_current(cgroup_path) / (1024 * 1024);
            long peak_mb = read_memory_peak(cgroup_path) / (1024 * 1024);
            
            steps[num_steps].step = i + 1;
            steps[num_steps].allocated_mb = target_mb;
            steps[num_steps].current_usage_mb = current_mb;
            steps[num_steps].peak_usage_mb = peak_mb;
            
            if (result == 0) {
                // Processo ainda rodando
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
                steps[num_steps].allocation_success = 1;
                steps[num_steps].status_message = "OK";
                max_allocated = target_mb;
            } else if (WIFEXITED(status)) {
                if (WEXITSTATUS(status) == 0) {
                    steps[num_steps].allocation_success = 1;
                    steps[num_steps].status_message = "OK";
                    max_allocated = target_mb;
                } else {
                    steps[num_steps].allocation_success = 0;
                    steps[num_steps].status_message = "FALHA";
                }
            } else if (WIFSIGNALED(status)) {
                // Processo foi morto (provavelmente OOM)
                steps[num_steps].allocation_success = 0;
                steps[num_steps].status_message = "OOM KILL";
                oom_killed = 1;
            } else {
                steps[num_steps].allocation_success = 0;
                steps[num_steps].status_message = "ERRO";
            }
            
            printf("%-6d | %15ld | %15ld | %15ld | %-10s\n",
                   steps[num_steps].step,
                   steps[num_steps].allocated_mb,
                   steps[num_steps].current_usage_mb,
                   steps[num_steps].peak_usage_mb,
                   steps[num_steps].status_message);
            fflush(stdout); // Flush após cada linha
            
            num_steps++;
            
            // Se houve OOM kill, parar
            if (oom_killed) {
                printf("\n✗ OOM Killer ativado - interrompendo teste\n");
                break;
            }
            
            // Se falhou a alocação, parar
            if (!steps[num_steps - 1].allocation_success) {
                printf("\n✗ Falha de alocação - limite atingido\n");
                break;
            }
            
            usleep(100000); // Pequena pausa entre alocações
        }
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("FASE 3: Análise de eventos de memória\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    long failcnt = read_memory_failcnt(cgroup_path);
    long final_peak = read_memory_peak(cgroup_path);
    
    printf("Eventos de pressão de memória:\n");
    printf("  • Eventos OOM/MAX: %ld\n", failcnt);
    printf("  • Pico de memória: %ld MB\n", final_peak / (1024 * 1024));
    printf("  • Quantidade máxima alocada: %ld MB\n", max_allocated);
    printf("  • OOM Killer ativado: %s\n", oom_killed ? "SIM" : "NÃO");
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("ANÁLISE E CONCLUSÕES\n");
    printf("═══════════════════════════════════════════════════════════════════════\n\n");
    
    printf("Comportamento observado:\n");
    if (oom_killed) {
        printf("  • OOM Killer foi ativado ao atingir o limite\n");
        printf("  • Processos foram terminados para liberar memória\n");
        printf("  • Comportamento: KILL (terminação forçada)\n");
    } else if (max_allocated >= MEMORY_LIMIT_MB) {
        printf("  • Limite foi respeitado sem OOM Kill\n");
        printf("  • Alocações próximas ao limite foram bem-sucedidas\n");
        printf("  • Comportamento: THROTTLE (controle de alocação)\n");
    } else {
        printf("  • Falhas de alocação antes do limite teórico\n");
        printf("  • Memória pode estar fragmentada ou em uso pelo sistema\n");
        printf("  • Comportamento: FAIL (falha de malloc)\n");
    }
    
    printf("\n");
    printf("Eficácia do limite:\n");
    double efficiency = (double)max_allocated / MEMORY_LIMIT_MB * 100.0;
    printf("  • Limite configurado: %d MB\n", MEMORY_LIMIT_MB);
    printf("  • Máximo alocado: %ld MB (%.1f%% do limite)\n", max_allocated, efficiency);
    
    if (failcnt > 0) {
        printf("  • Eventos de falha: %ld\n", failcnt);
        printf("  • Cgroup v2 ATIVO - limites sendo aplicados\n");
    } else {
        printf("  • Sem eventos de falha registrados\n");
    }
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    
    // Exportar resultados
    printf("\nExportando resultados para arquivo...\n");
    
    FILE *csv = fopen("output/experiment4_memory_limit.csv", "w");
    if (csv) {
        fprintf(csv, "step,target_mb,current_mb,peak_mb,success,status\n");
        
        for (int i = 0; i < num_steps; i++) {
            fprintf(csv, "%d,%ld,%ld,%ld,%d,%s\n",
                    steps[i].step,
                    steps[i].allocated_mb,
                    steps[i].current_usage_mb,
                    steps[i].peak_usage_mb,
                    steps[i].allocation_success,
                    steps[i].status_message);
        }
        
        fclose(csv);
        printf("✓ Resultados salvos em: output/experiment4_memory_limit.csv\n");
    }
    
    FILE *json = fopen("output/experiments/exp4_memory_limit.json", "w");
    if (json) {
        fprintf(json, "{\n");
        fprintf(json, "  \"experiment\": \"Memory Limit with Cgroups v2\",\n");
        fprintf(json, "  \"date\": \"%s\",\n", get_iso_timestamp());
        fprintf(json, "  \"configuration\": {\n");
        fprintf(json, "    \"memory_limit_mb\": %d,\n", MEMORY_LIMIT_MB);
        fprintf(json, "    \"allocation_step_mb\": %d,\n", ALLOCATION_STEP_MB);
        fprintf(json, "    \"max_allocations\": %d\n", MAX_ALLOCATIONS);
        fprintf(json, "  },\n");
        fprintf(json, "  \"results\": {\n");
        fprintf(json, "    \"max_allocated_mb\": %ld,\n", max_allocated);
        fprintf(json, "    \"peak_usage_mb\": %ld,\n", final_peak / (1024 * 1024));
        fprintf(json, "    \"memory_failcnt\": %ld,\n", failcnt);
        fprintf(json, "    \"oom_killed\": %s,\n", oom_killed ? "true" : "false");
        fprintf(json, "    \"num_steps\": %d\n", num_steps);
        fprintf(json, "  },\n");
        fprintf(json, "  \"behavior\": \"%s\",\n", 
                oom_killed ? "OOM_KILL" : (max_allocated >= MEMORY_LIMIT_MB ? "THROTTLE" : "FAIL"));
        fprintf(json, "  \"steps\": [\n");
        
        for (int i = 0; i < num_steps; i++) {
            fprintf(json, "    {\n");
            fprintf(json, "      \"step\": %d,\n", steps[i].step);
            fprintf(json, "      \"target_mb\": %ld,\n", steps[i].allocated_mb);
            fprintf(json, "      \"current_mb\": %ld,\n", steps[i].current_usage_mb);
            fprintf(json, "      \"peak_mb\": %ld,\n", steps[i].peak_usage_mb);
            fprintf(json, "      \"success\": %s,\n", steps[i].allocation_success ? "true" : "false");
            fprintf(json, "      \"status\": \"%s\"\n", steps[i].status_message);
            fprintf(json, "    }%s\n", i < num_steps - 1 ? "," : "");
        }
        
        fprintf(json, "  ]\n");
        fprintf(json, "}\n");
        fclose(json);
        printf("✓ Relatório JSON salvo em: output/experiments/exp4_memory_limit.json\n");
    }
    
    // Limpar
    rmdir(cgroup_path);
    
    printf("\n");
}
