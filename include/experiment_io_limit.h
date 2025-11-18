#ifndef EXPERIMENT_IO_LIMIT_H
#define EXPERIMENT_IO_LIMIT_H

/**
 * Experimento 5: Limitação de I/O
 * 
 * Objetivo: Avaliar precisão de limitação de I/O
 * 
 * Procedimento:
 * 1. Executar workload I/O-intensive sem limite
 * 2. Aplicar limites de read/write BPS
 * 3. Medir throughput real
 * 
 * Métricas reportadas:
 * • Throughput medido vs limite configurado
 * • Latência de I/O
 * • Impacto no tempo total de execução
 */

void run_experiment_io_limit_new();

#endif // EXPERIMENT_IO_LIMIT_H
