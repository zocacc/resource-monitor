#include "../include/monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static bool read_io_stats(int pid, ResourceData *data) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/proc/%d/io", pid);

    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        // This file is only readable by root or the process owner
        // We'll return true but with 0 values if we can't open it.
        data->io_read_bytes = 0;
        data->io_write_bytes = 0;
        // This is not a fatal error for the whole monitor, so we don't return false
        // unless the PID itself is invalid, which is checked by other functions.
        return true; 
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "read_bytes:", 11) == 0) {
            sscanf(line + 12, "%lld", &data->io_read_bytes);
        } else if (strncmp(line, "write_bytes:", 12) == 0) {
            sscanf(line + 13, "%lld", &data->io_write_bytes);
        } else if (strncmp(line, "syscr:", 6) == 0) {
            sscanf(line + 7, "%lld", &data->io_read_syscalls);
        } else if (strncmp(line, "syscw:", 6) == 0) {
            sscanf(line + 7, "%lld", &data->io_write_syscalls);
        }
    }

    fclose(fp);
    return true;
}

bool get_io_data(int pid, ResourceData *data) {
    data->pid = pid;
    data->timestamp = time(NULL);
    return read_io_stats(pid, data);
}
