#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include "../include/cgroup.h"

#define CGROUP_V2_ROOT "/sys/fs/cgroup"

int read_cpu_stat_v2(const char *cgroup_name) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/cpu.stat", CGROUP_V2_ROOT, cgroup_name);
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Erro ao abrir %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║              CPU Statistics - %s%-*s║\n", cgroup_name, 
           (int)(26 - strlen(cgroup_name)), "");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char key[64];
        unsigned long long value;
        if (sscanf(line, "%s %llu", key, &value) == 2) {
            printf("║ %-30s %25llu  ║\n", key, value);
        }
    }
    
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    
    fclose(fp);
    return 0;
}

int read_memory_stat_v2(const char *cgroup_name) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/memory.stat", CGROUP_V2_ROOT, cgroup_name);
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Erro ao abrir %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║            Memory Statistics - %s%-*s║\n", cgroup_name, 
           (int)(23 - strlen(cgroup_name)), "");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char key[64];
        unsigned long long value;
        if (sscanf(line, "%s %llu", key, &value) == 2) {
            if (value > 1024 * 1024 * 1024) {
                printf("║ %-30s %20.2f GB  ║\n", key, value / (1024.0 * 1024.0 * 1024.0));
            } else if (value > 1024 * 1024) {
                printf("║ %-30s %20.2f MB  ║\n", key, value / (1024.0 * 1024.0));
            } else if (value > 1024) {
                printf("║ %-30s %20.2f KB  ║\n", key, value / 1024.0);
            } else {
                printf("║ %-30s %20llu B   ║\n", key, value);
            }
        }
    }
    
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    
    fclose(fp);
    return 0;
}

int read_io_stat_v2(const char *cgroup_name) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/io.stat", CGROUP_V2_ROOT, cgroup_name);
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Erro ao abrir %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║              I/O Statistics - %s%-*s║\n", cgroup_name, 
           (int)(24 - strlen(cgroup_name)), "");
    printf("╠═══════════════════════════════════════════════════════════╣\n");
    
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        printf("║ %-57s║\n", line);
    }
    
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    
    fclose(fp);
    return 0;
}

int read_pids_stat_v2(const char *cgroup_name) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/pids.current", CGROUP_V2_ROOT, cgroup_name);
    
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Erro ao abrir %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    int current;
    if (fscanf(fp, "%d", &current) == 1) {
        printf("PIDs atuais: %d\n", current);
    }
    
    fclose(fp);
    
    snprintf(path, sizeof(path), "%s/%s/pids.max", CGROUP_V2_ROOT, cgroup_name);
    fp = fopen(path, "r");
    if (fp) {
        char max[32];
        if (fgets(max, sizeof(max), fp)) {
            printf("PIDs máximo: %s", max);
        }
        fclose(fp);
    }
    
    return 0;
}

int set_pids_max_v2(const char *cgroup_name, int max_pids) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/pids.max", CGROUP_V2_ROOT, cgroup_name);
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "Erro ao abrir %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    fprintf(fp, "%d\n", max_pids);
    fclose(fp);
    
    printf("Limite de PIDs definido para %d no cgroup %s\n", max_pids, cgroup_name);
    return 0;
}

int set_cpu_quota(const char *group_name, int period_us, int quota_us) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/cpu.max", CGROUP_V2_ROOT, group_name);
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "Erro ao abrir %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    fprintf(fp, "%d %d\n", quota_us, period_us);
    fclose(fp);
    
    return 0;
}

int set_memory_limit(const char *group_name, long long limit_bytes) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/memory.max", CGROUP_V2_ROOT, group_name);
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "Erro ao abrir %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    fprintf(fp, "%lld\n", limit_bytes);
    fclose(fp);
    
    return 0;
}

int add_process_to_cgroup(int pid, const char *controller, const char *group_name) {
    (void)controller; // Unused in v2
    
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/cgroup.procs", CGROUP_V2_ROOT, group_name);
    
    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "Erro ao abrir %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    fprintf(fp, "%d\n", pid);
    fclose(fp);
    
    return 0;
}

void export_cgroup_info_to_json(const char *controller, const char *group_name, const char *filename) {
    (void)controller; // Unused in v2
    
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "Erro ao criar arquivo %s: %s\n", filename, strerror(errno));
        return;
    }
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"cgroup\": \"%s\",\n", group_name);
    fprintf(fp, "  \"timestamp\": %ld,\n", time(NULL));
    
    // CPU stats
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/cpu.stat", CGROUP_V2_ROOT, group_name);
    FILE *cpu = fopen(path, "r");
    if (cpu) {
        fprintf(fp, "  \"cpu\": {\n");
        char line[256];
        int first = 1;
        while (fgets(line, sizeof(line), cpu)) {
            char key[64];
            unsigned long long value;
            if (sscanf(line, "%s %llu", key, &value) == 2) {
                if (!first) fprintf(fp, ",\n");
                fprintf(fp, "    \"%s\": %llu", key, value);
                first = 0;
            }
        }
        fprintf(fp, "\n  },\n");
        fclose(cpu);
    }
    
    // Memory stats
    snprintf(path, sizeof(path), "%s/%s/memory.stat", CGROUP_V2_ROOT, group_name);
    FILE *mem = fopen(path, "r");
    if (mem) {
        fprintf(fp, "  \"memory\": {\n");
        char line[256];
        int first = 1;
        while (fgets(line, sizeof(line), mem)) {
            char key[64];
            unsigned long long value;
            if (sscanf(line, "%s %llu", key, &value) == 2) {
                if (!first) fprintf(fp, ",\n");
                fprintf(fp, "    \"%s\": %llu", key, value);
                first = 0;
            }
        }
        fprintf(fp, "\n  }\n");
        fclose(mem);
    }
    
    fprintf(fp, "}\n");
    fclose(fp);
}
