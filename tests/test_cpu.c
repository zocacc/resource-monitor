#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "../include/monitor.h"

void test_cpu_monitor() {
    printf("\n--- Executando Teste de CPU Monitor ---\n");
    ResourceData data;
    bool success;

    // Tenta monitorar um PID inexistente (espera-se falha)
    success = get_cpu_data(999999, &data);
    if (!success) {
        printf("Teste PID inexistente: PASSED (função retornou falha como esperado)\n");
    } else {
        printf("Teste PID inexistente: FAILED (função retornou sucesso inesperadamente)\n");
    }

    // Tenta monitorar o próprio processo (PID atual)
    int current_pid = getpid();
    printf("Monitorando PID atual: %d\n", current_pid);

    ResourceData initial_data;
    success = get_cpu_data(current_pid, &initial_data);
    if (!success) {
        printf("Teste de coleta inicial: FAILED (não foi possível ler os dados do PID atual)\n");
        return;
    }
    printf("  CPU User (inicial): %ld, CPU System (inicial): %ld\n", initial_data.cpu_user, initial_data.cpu_system);

    // Simula algum trabalho para consumir CPU
    long i, j;
    for (i = 0; i < 100000; i++) {
        for (j = 0; j < 1000; j++) {
            // Loop vazio para consumir CPU
        }
    }

    ResourceData final_data;
    success = get_cpu_data(current_pid, &final_data);
    if (!success) {
        printf("Teste de coleta final: FAILED (não foi possível ler os dados do PID atual)\n");
        return;
    }
    printf("  CPU User (final): %ld, CPU System (final): %ld\n", final_data.cpu_user, final_data.cpu_system);

    if (final_data.cpu_user >= initial_data.cpu_user) {
        printf("Teste consumo de CPU: PASSED (valores de CPU aumentaram ou permaneceram)\n");
    } else {
        printf("Teste consumo de CPU: FAILED (valores de CPU diminuíram inesperadamente)\n");
    }

    printf("--- Teste de CPU Monitor Concluído ---\n");
}

int main() {
    test_cpu_monitor();
    return 0;
}
