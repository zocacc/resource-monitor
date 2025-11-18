#include "../include/monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Lê métricas do /proc/[pid]/statm
static bool read_statm_metrics(int pid, ResourceData *data) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/proc/%d/statm", pid);

    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        return false;
    }

    // Campos: size (VSZ), resident (RSS)
    // Valores em páginas.
    long vsz_pages;
    fscanf(fp, "%ld %ld", &vsz_pages, &data->memory_rss);
    fclose(fp);

    // Converte VSZ de páginas para KB
    long page_size_kb = sysconf(_SC_PAGESIZE) / 1024;
    data->memory_vsz = vsz_pages * page_size_kb;

    return true;
}

// Lê métricas do /proc/[pid]/status
static bool read_status_metrics(int pid, ResourceData *data) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/proc/%d/status", pid);

    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        return false;
    }

    char line[256];
    data->memory_swap = 0; // Inicializa caso não encontre
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmSwap:", 7) == 0) {
            sscanf(line + 8, "%ld", &data->memory_swap);
        }
    }

    fclose(fp);
    return true;
}

bool get_memory_data(int pid, ResourceData *data) {
    data->pid = pid;
    data->timestamp = time(NULL);

    if (!read_statm_metrics(pid, data)) {
        return false;
    }
    if (!read_status_metrics(pid, data)) {
        // Process might have died, but we have statm data
    }

    return true;
}
