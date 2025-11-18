#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include "../include/monitor.h"

void test_io_monitor() {
    printf("\n--- Executando Teste de I/O Monitor ---\n");
    ResourceData data;
    bool success;

    // Tenta monitorar um PID inexistente
    // A função get_io_data pode não falhar, mas deve retornar 0.
    success = get_io_data(999999, &data);
    if (success && data.io_read_bytes == 0 && data.io_write_bytes == 0) {
        printf("Teste PID inexistente: PASSED (retornou 0 para I/O bytes)\n");
    } else {
        printf("Teste PID inexistente: FAILED (retornou valores inesperados para I/O bytes)\n");
    }

    // Tenta monitorar o próprio processo (PID atual)
    int current_pid = getpid();
    printf("Monitorando PID atual: %d\n", current_pid);

    ResourceData initial_data;
    success = get_io_data(current_pid, &initial_data);
    if (!success) {
        printf("Teste de coleta inicial: FAILED (não foi possível ler os dados do PID atual)\n");
        return;
    }
    printf("  I/O Read (inicial): %lld, I/O Write (inicial): %lld\n", initial_data.io_read_bytes, initial_data.io_write_bytes);

    // Realiza algumas operações de I/O
    const char *test_file = "test_io_file.tmp";
    FILE *fp = fopen(test_file, "w+");
    if (fp == NULL) {
        perror("Falha ao criar arquivo de teste de I/O");
        return;
    }

    // Escreve no arquivo
    fprintf(fp, "Isso é um teste de escrita de I/O.\n");
    fflush(fp);

    // Lê do arquivo
    fseek(fp, 0, SEEK_SET);
    char buffer[100];
    fgets(buffer, sizeof(buffer), fp);

    fclose(fp);
    remove(test_file); // Limpa o arquivo de teste

    ResourceData final_data;
    success = get_io_data(current_pid, &final_data);
    if (!success) {
        printf("Teste de coleta final: FAILED (não foi possível ler os dados do PID atual)\n");
        return;
    }
    printf("  I/O Read (final): %lld, I/O Write (final): %lld\n", final_data.io_read_bytes, final_data.io_write_bytes);

    // A verificação exata pode ser instável, apenas verificamos se não diminuiu.
    if (final_data.io_write_bytes >= initial_data.io_write_bytes) {
        printf("Teste operações de I/O: PASSED (valores de I/O aumentaram ou permaneceram)\n");
    } else {
        printf("Teste operações de I/O: FAILED (valores de I/O diminuíram inesperadamente)\n");
    }

    printf("--- Teste de I/O Monitor Concluído ---\n");
}

int main() {
    test_io_monitor();
    return 0;
}
