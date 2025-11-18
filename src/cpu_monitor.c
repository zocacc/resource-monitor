#include "../include/monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Lê métricas do /proc/[pid]/stat
static bool read_stat_metrics(int pid, ResourceData *data) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/proc/%d/stat", pid);

    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        return false;
    }

    // Campos de interesse:
    // 10: minflt  (page_faults_minor)
    // 12: majflt  (page_faults_major)
    // 14: utime   (cpu_user)
    // 15: stime   (cpu_system)
    // 20: num_threads
    fscanf(fp, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %lu %*u %lu %*u %lu %lu %*d %*d %*d %*d %ld",
           &data->page_faults_minor,
           &data->page_faults_major,
           &data->cpu_user,
           &data->cpu_system,
           &data->num_threads);
    
    fclose(fp);
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
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "voluntary_ctxt_switches:", 24) == 0) {
            sscanf(line + 25, "%ld", &data->voluntary_context_switches);
        } else if (strncmp(line, "nonvoluntary_ctxt_switches:", 27) == 0) {
            sscanf(line + 28, "%ld", &data->nonvoluntary_context_switches);
        }
    }

    fclose(fp);
    return true;
}

bool get_cpu_data(int pid, ResourceData *data) {
    data->pid = pid;
    data->timestamp = time(NULL);

    if (!read_stat_metrics(pid, data)) {
        return false;
    }
    if (!read_status_metrics(pid, data)) {
        // This might fail if the process dies between reads, but we can live with it
        // We'll still have the data from /stat
    }

    return true;
}
