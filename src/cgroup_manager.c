#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>

#define CGROUP_ROOT "/sys/fs/cgroup"
#define MAX_PATH 512
#define MAX_LINE 1024

// Estrutura para armazenar métricas
typedef struct {
    unsigned long cpu_usage_usec;
    unsigned long cpu_user_usec;
    unsigned long cpu_system_usec;
    unsigned long nr_periods;
    unsigned long nr_throttled;
    unsigned long throttled_usec;
} CPUStat;

typedef struct {
    unsigned long current;
    unsigned long max;
    unsigned long peak;
    unsigned long anon;
    unsigned long file;
} MemoryStat;

typedef struct {
    unsigned long rbytes;
    unsigned long wbytes;
    unsigned long rios;
    unsigned long wios;
} IOStat;

// Função para ler arquivo cgroup
int read_cgroup_file(const char *cgroup_path, const char *file, char *buffer, size_t size) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s/%s", CGROUP_ROOT, cgroup_path, file);
    
    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }
    
    size_t bytes = fread(buffer, 1, size - 1, f);
    buffer[bytes] = '\0';
    fclose(f);
    return 0;
}

// Função para escrever em arquivo cgroup
int write_cgroup_file(const char *cgroup_path, const char *file, const char *value) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s/%s", CGROUP_ROOT, cgroup_path, file);
    
    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "Erro ao abrir %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    fprintf(f, "%s", value);
    fclose(f);
    return 0;
}

// Ler métricas de CPU
int read_cpu_metrics(const char *cgroup_name, CPUStat *stat) {
    char buffer[MAX_LINE * 10];
    
    if (read_cgroup_file(cgroup_name, "cpu.stat", buffer, sizeof(buffer)) != 0) {
        fprintf(stderr, "Erro ao ler cpu.stat do cgroup '%s'\n", cgroup_name);
        return -1;
    }
    
    memset(stat, 0, sizeof(CPUStat));
    
    char *line = strtok(buffer, "\n");
    while (line) {
        if (sscanf(line, "usage_usec %lu", &stat->cpu_usage_usec) == 1) {}
        else if (sscanf(line, "user_usec %lu", &stat->cpu_user_usec) == 1) {}
        else if (sscanf(line, "system_usec %lu", &stat->cpu_system_usec) == 1) {}
        else if (sscanf(line, "nr_periods %lu", &stat->nr_periods) == 1) {}
        else if (sscanf(line, "nr_throttled %lu", &stat->nr_throttled) == 1) {}
        else if (sscanf(line, "throttled_usec %lu", &stat->throttled_usec) == 1) {}
        
        line = strtok(NULL, "\n");
    }
    
    return 0;
}

// Ler métricas de memória
int read_memory_metrics(const char *cgroup_name, MemoryStat *stat) {
    char buffer[MAX_LINE];
    
    memset(stat, 0, sizeof(MemoryStat));
    
    // Ler memory.current
    if (read_cgroup_file(cgroup_name, "memory.current", buffer, sizeof(buffer)) == 0) {
        sscanf(buffer, "%lu", &stat->current);
    }
    
    // Ler memory.max
    if (read_cgroup_file(cgroup_name, "memory.max", buffer, sizeof(buffer)) == 0) {
        if (strcmp(buffer, "max\n") == 0) {
            stat->max = 0; // Sem limite
        } else {
            sscanf(buffer, "%lu", &stat->max);
        }
    }
    
    // Ler memory.peak
    if (read_cgroup_file(cgroup_name, "memory.peak", buffer, sizeof(buffer)) == 0) {
        sscanf(buffer, "%lu", &stat->peak);
    }
    
    // Ler memory.stat para anon e file
    if (read_cgroup_file(cgroup_name, "memory.stat", buffer, sizeof(buffer)) == 0) {
        char *line = strtok(buffer, "\n");
        while (line) {
            if (sscanf(line, "anon %lu", &stat->anon) == 1) {}
            else if (sscanf(line, "file %lu", &stat->file) == 1) {}
            line = strtok(NULL, "\n");
        }
    }
    
    return 0;
}

// Ler métricas de I/O
int read_io_metrics(const char *cgroup_name, IOStat *stat) {
    char buffer[MAX_LINE * 20];
    
    memset(stat, 0, sizeof(IOStat));
    
    if (read_cgroup_file(cgroup_name, "io.stat", buffer, sizeof(buffer)) != 0) {
        return -1;
    }
    
    char *line = strtok(buffer, "\n");
    while (line) {
        unsigned long rbytes, wbytes, rios, wios;
        if (sscanf(line, "%*s rbytes=%lu wbytes=%lu rios=%lu wios=%lu", 
                   &rbytes, &wbytes, &rios, &wios) == 4) {
            stat->rbytes += rbytes;
            stat->wbytes += wbytes;
            stat->rios += rios;
            stat->wios += wios;
        }
        line = strtok(NULL, "\n");
    }
    
    return 0;
}

// Criar cgroup experimental
int create_cgroup(const char *cgroup_name) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", CGROUP_ROOT, cgroup_name);
    
    if (mkdir(path, 0755) != 0) {
        if (errno == EEXIST) {
            printf("Cgroup '%s' já existe\n", cgroup_name);
            return 0;
        }
        fprintf(stderr, "Erro ao criar cgroup '%s': %s\n", cgroup_name, strerror(errno));
        return -1;
    }
    
    printf("Cgroup '%s' criado com sucesso em %s\n", cgroup_name, path);
    return 0;
}

// Mover processo para cgroup
int move_process_to_cgroup(const char *cgroup_name, pid_t pid) {
    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d", pid);
    
    if (write_cgroup_file(cgroup_name, "cgroup.procs", pid_str) != 0) {
        fprintf(stderr, "Erro ao mover processo %d para cgroup '%s'\n", pid, cgroup_name);
        return -1;
    }
    
    printf("Processo %d movido para cgroup '%s'\n", pid, cgroup_name);
    return 0;
}

// Aplicar limite de CPU
int set_cpu_limit(const char *cgroup_name, unsigned long quota_us, unsigned long period_us) {
    char value[64];
    snprintf(value, sizeof(value), "%lu %lu", quota_us, period_us);
    
    if (write_cgroup_file(cgroup_name, "cpu.max", value) != 0) {
        fprintf(stderr, "Erro ao configurar limite de CPU\n");
        return -1;
    }
    
    printf("Limite de CPU configurado: %lu/%lu (%.1f%%)\n", 
           quota_us, period_us, (quota_us * 100.0) / period_us);
    return 0;
}

// Aplicar limite de memória
int set_memory_limit(const char *cgroup_name, unsigned long limit_bytes) {
    char value[64];
    snprintf(value, sizeof(value), "%lu", limit_bytes);
    
    if (write_cgroup_file(cgroup_name, "memory.max", value) != 0) {
        fprintf(stderr, "Erro ao configurar limite de memória\n");
        return -1;
    }
    
    printf("Limite de memória configurado: %lu bytes (%.2f MB)\n", 
           limit_bytes, limit_bytes / 1048576.0);
    return 0;
}

// Gerar relatório de utilização
int generate_report(const char *cgroup_name, const char *output_file) {
    CPUStat cpu;
    MemoryStat mem;
    IOStat io;
    
    printf("\n");
    printf("=================================================================\n");
    printf("           RELATORIO DE UTILIZACAO DO CGROUP\n");
    printf("=================================================================\n");
    printf("Cgroup: %s\n", cgroup_name);
    printf("Timestamp: %ld\n", time(NULL));
    printf("=================================================================\n");
    
    // CPU
    if (read_cpu_metrics(cgroup_name, &cpu) == 0) {
        printf("\n[CPU]\n");
        printf("  Total Usage:     %lu us (%.2f s)\n", cpu.cpu_usage_usec, cpu.cpu_usage_usec / 1000000.0);
        printf("  User Time:       %lu us\n", cpu.cpu_user_usec);
        printf("  System Time:     %lu us\n", cpu.cpu_system_usec);
        printf("  Periods:         %lu\n", cpu.nr_periods);
        printf("  Throttled:       %lu periods\n", cpu.nr_throttled);
        printf("  Throttled Time:  %lu us (%.2f s)\n", cpu.throttled_usec, cpu.throttled_usec / 1000000.0);
        if (cpu.nr_periods > 0) {
            printf("  Throttle Rate:   %.2f%%\n", (cpu.nr_throttled * 100.0) / cpu.nr_periods);
        }
    }
    
    // Memory
    if (read_memory_metrics(cgroup_name, &mem) == 0) {
        printf("\n[MEMORY]\n");
        printf("  Current:         %lu bytes (%.2f MB)\n", mem.current, mem.current / 1048576.0);
        if (mem.max > 0) {
            printf("  Max Limit:       %lu bytes (%.2f MB)\n", mem.max, mem.max / 1048576.0);
            printf("  Usage:           %.2f%%\n", (mem.current * 100.0) / mem.max);
        } else {
            printf("  Max Limit:       unlimited\n");
        }
        printf("  Peak:            %lu bytes (%.2f MB)\n", mem.peak, mem.peak / 1048576.0);
        printf("  Anonymous:       %lu bytes (%.2f MB)\n", mem.anon, mem.anon / 1048576.0);
        printf("  File Cache:      %lu bytes (%.2f MB)\n", mem.file, mem.file / 1048576.0);
    }
    
    // I/O
    if (read_io_metrics(cgroup_name, &io) == 0) {
        printf("\n[I/O - BlkIO]\n");
        printf("  Read Bytes:      %lu (%.2f MB)\n", io.rbytes, io.rbytes / 1048576.0);
        printf("  Write Bytes:     %lu (%.2f MB)\n", io.wbytes, io.wbytes / 1048576.0);
        printf("  Read Ops:        %lu\n", io.rios);
        printf("  Write Ops:       %lu\n", io.wios);
    }
    
    // Processos no cgroup
    char procs_path[MAX_PATH];
    snprintf(procs_path, sizeof(procs_path), "%s/%s/cgroup.procs", CGROUP_ROOT, cgroup_name);
    FILE *procs_file = fopen(procs_path, "r");
    if (procs_file) {
        printf("\n[PROCESSOS]\n");
        char line[64];
        int count = 0;
        int pids[100];  // Armazenar até 100 PIDs
        
        while (fgets(line, sizeof(line), procs_file) && count < 100) {
            pids[count] = atoi(line);
            count++;
        }
        fclose(procs_file);
        
        printf("  Total: %d processo(s)\n", count);
        if (count > 0) {
            printf("\n  PID    | Command\n");
            printf("  -------+-------------------------------------------------\n");
            for (int i = 0; i < count; i++) {
                char cmd_path[256];
                char cmd[256] = {0};
                snprintf(cmd_path, sizeof(cmd_path), "/proc/%d/comm", pids[i]);
                FILE *cmd_file = fopen(cmd_path, "r");
                if (cmd_file) {
                    if (fgets(cmd, sizeof(cmd), cmd_file)) {
                        // Remover newline
                        cmd[strcspn(cmd, "\n")] = 0;
                    }
                    fclose(cmd_file);
                } else {
                    snprintf(cmd, sizeof(cmd), "<não acessível>");
                }
                printf("  %-6d | %s\n", pids[i], cmd);
            }
        }
    }
    
    printf("\n=================================================================\n\n");
    
    // Exportar para JSON se solicitado
    if (output_file) {
        FILE *f = fopen(output_file, "w");
        if (f) {
            fprintf(f, "{\n");
            fprintf(f, "  \"cgroup\": \"%s\",\n", cgroup_name);
            fprintf(f, "  \"timestamp\": %ld,\n", time(NULL));
            fprintf(f, "  \"cpu\": {\n");
            fprintf(f, "    \"usage_usec\": %lu,\n", cpu.cpu_usage_usec);
            fprintf(f, "    \"user_usec\": %lu,\n", cpu.cpu_user_usec);
            fprintf(f, "    \"system_usec\": %lu,\n", cpu.cpu_system_usec);
            fprintf(f, "    \"nr_periods\": %lu,\n", cpu.nr_periods);
            fprintf(f, "    \"nr_throttled\": %lu,\n", cpu.nr_throttled);
            fprintf(f, "    \"throttled_usec\": %lu\n", cpu.throttled_usec);
            fprintf(f, "  },\n");
            fprintf(f, "  \"memory\": {\n");
            fprintf(f, "    \"current\": %lu,\n", mem.current);
            fprintf(f, "    \"max\": %lu,\n", mem.max);
            fprintf(f, "    \"peak\": %lu,\n", mem.peak);
            fprintf(f, "    \"anon\": %lu,\n", mem.anon);
            fprintf(f, "    \"file\": %lu\n", mem.file);
            fprintf(f, "  },\n");
            fprintf(f, "  \"io\": {\n");
            fprintf(f, "    \"rbytes\": %lu,\n", io.rbytes);
            fprintf(f, "    \"wbytes\": %lu,\n", io.wbytes);
            fprintf(f, "    \"rios\": %lu,\n", io.rios);
            fprintf(f, "    \"wios\": %lu\n", io.wios);
            fprintf(f, "  }\n");
            fprintf(f, "}\n");
            fclose(f);
            printf("Relatorio JSON salvo em: %s\n\n", output_file);
        }
    }
    
    return 0;
}

// Listar todos os cgroups do sistema
int list_all_cgroups() {
    DIR *dir = opendir(CGROUP_ROOT);
    if (!dir) {
        fprintf(stderr, "Erro ao abrir %s: %s\n", CGROUP_ROOT, strerror(errno));
        return -1;
    }
    
    printf("\n");
    printf("=================================================================\n");
    printf("                  CGROUPS DISPONIVEIS NO SISTEMA\n");
    printf("=================================================================\n");
    printf("Path: %s\n", CGROUP_ROOT);
    printf("=================================================================\n\n");
    
    printf("CGROUP RAIZ - Arquivos de controle principais:\n");
    printf("  cgroup.controllers, cgroup.procs, cgroup.subtree_control\n");
    printf("  cpu.stat, memory.stat, io.stat, etc.\n\n");
    
    printf("SUB-CGROUPS (diretorios):\n\n");
    
    int count = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        // Pular . e ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", CGROUP_ROOT, entry->d_name);
        
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            count++;
            printf("  %3d) %-40s", count, entry->d_name);
            
            // Tentar ler quantos processos estão no cgroup
            char procs_path[1024];
            snprintf(procs_path, sizeof(procs_path), "%s/cgroup.procs", path);
            FILE *f = fopen(procs_path, "r");
            if (f) {
                int proc_count = 0;
                char line[64];
                while (fgets(line, sizeof(line), f)) {
                    proc_count++;
                }
                fclose(f);
                printf(" [%d processos]", proc_count);
            }
            
            printf("\n");
        }
    }
    
    closedir(dir);
    
    printf("\n=================================================================\n");
    printf("Total: %d sub-cgroups encontrados\n", count);
    printf("=================================================================\n\n");
    
    return 0;
}

void print_usage(const char *prog_name) {
    printf("Uso: %s <comando> [opcoes]\n\n", prog_name);
    printf("Comandos:\n");
    printf("  list                                          - Listar todos os cgroups do sistema\n");
    printf("  read-metrics <cgroup_name>                    - Ler metricas de CPU, Memory e BlkIO\n");
    printf("  create <cgroup_name>                          - Criar cgroup experimental\n");
    printf("  move-process <cgroup_name> <pid>              - Mover processo para cgroup\n");
    printf("  set-cpu-limit <cgroup_name> <quota> <period>  - Aplicar limite de CPU (em microsegundos)\n");
    printf("  set-mem-limit <cgroup_name> <bytes>           - Aplicar limite de memoria\n");
    printf("  report <cgroup_name> [output.json]            - Gerar relatorio de utilizacao\n");
    printf("\n");
    printf("Exemplos:\n");
    printf("  %s list\n", prog_name);
    printf("  %s create my_cgroup\n", prog_name);
    printf("  %s set-cpu-limit my_cgroup 50000 100000  # 50%% CPU\n", prog_name);
    printf("  %s set-mem-limit my_cgroup 104857600     # 100 MB\n", prog_name);
    printf("  %s move-process my_cgroup 1234\n", prog_name);
    printf("  %s report my_cgroup output/report.json\n", prog_name);
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char *command = argv[1];
    
    // Verificar se é root
    if (geteuid() != 0) {
        fprintf(stderr, "AVISO: Este programa requer privilegios root (sudo)\n");
    }
    
    if (strcmp(command, "list") == 0) {
        return list_all_cgroups();
        
    } else if (strcmp(command, "read-metrics") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Uso: %s read-metrics <cgroup_name>\n", argv[0]);
            return 1;
        }
        return generate_report(argv[2], NULL);
        
    } else if (strcmp(command, "create") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Uso: %s create <cgroup_name>\n", argv[0]);
            return 1;
        }
        return create_cgroup(argv[2]);
        
    } else if (strcmp(command, "move-process") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Uso: %s move-process <cgroup_name> <pid>\n", argv[0]);
            return 1;
        }
        return move_process_to_cgroup(argv[2], atoi(argv[3]));
        
    } else if (strcmp(command, "set-cpu-limit") == 0) {
        if (argc != 5) {
            fprintf(stderr, "Uso: %s set-cpu-limit <cgroup_name> <quota_us> <period_us>\n", argv[0]);
            return 1;
        }
        return set_cpu_limit(argv[2], atol(argv[3]), atol(argv[4]));
        
    } else if (strcmp(command, "set-mem-limit") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Uso: %s set-mem-limit <cgroup_name> <limit_bytes>\n", argv[0]);
            return 1;
        }
        return set_memory_limit(argv[2], atol(argv[3]));
        
    } else if (strcmp(command, "report") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Uso: %s report <cgroup_name> [output.json]\n", argv[0]);
            return 1;
        }
        const char *output = (argc >= 4) ? argv[3] : NULL;
        return generate_report(argv[2], output);
        
    } else {
        fprintf(stderr, "Comando desconhecido: %s\n", command);
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}
