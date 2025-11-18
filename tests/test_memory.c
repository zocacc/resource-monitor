#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "../include/monitor.h"

void test_memory_monitor() {
    printf("\n--- Executando Teste de Memory Monitor ---\n");
    ResourceData data;
    bool success;

    // Tenta monitorar um PID inexistente
    success = get_memory_data(999999, &data);
    if (!success) {
        printf("Teste PID inexistente: PASSED (função retornou falha como esperado)\n");
    } else {
        printf("Teste PID inexistente: FAILED (função retornou sucesso inesperadamente)\n");
    }

    // Tenta monitorar o próprio processo (PID atual)
    int current_pid = getpid();
    printf("Monitorando PID atual: %d\n", current_pid);

    ResourceData initial_data;
    success = get_memory_data(current_pid, &initial_data);
    if (!success) {
        printf("Teste de coleta inicial: FAILED (não foi possível ler os dados do PID atual)\n");
        return;
    }
    printf("  Memory RSS (inicial): %ld pages\n", initial_data.memory_rss);

    // Aloca alguma memória para ver se o RSS aumenta
    size_t allocation_size = 10 * 1024 * 1024; // 10 MB
    char *memory_block = (char *)malloc(allocation_size);
    if (memory_block == NULL) {
        perror("Falha ao alocar memória para o teste");
        return;
    }
    // Toca na memória para garantir que seja residente
    for (size_t i = 0; i < allocation_size; i += 4096) { // A cada página
        memory_block[i] = 'A';
    }

    ResourceData final_data;
    success = get_memory_data(current_pid, &final_data);
    if (!success) {
        printf("Teste de coleta final: FAILED (não foi possível ler os dados do PID atual)\n");
        free(memory_block);
        return;
    }
    printf("  Memory RSS (final): %ld pages\n", final_data.memory_rss);

    if (final_data.memory_rss > initial_data.memory_rss) {
        printf("Teste alocação de memória: PASSED (RSS aumentou)\n");
    } else {
        printf("Teste alocação de memória: FAILED (RSS não aumentou como esperado)\n");
    }

    free(memory_block);
    printf("--- Teste de Memory Monitor Concluído ---\n");
}

int main() {
    test_memory_monitor();
    return 0;
}
