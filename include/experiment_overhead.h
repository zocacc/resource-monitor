#ifndef EXPERIMENT_OVERHEAD_H
#define EXPERIMENT_OVERHEAD_H

/**
 * Experimento 1: Overhead de Monitoramento
 * 
 * Objetivo: Medir o impacto do próprio profiler no sistema
 * 
 * Procedimento:
 * 1. Executar workload de referência sem monitoramento
 * 2. Executar mesmo workload com monitoramento em diferentes intervalos
 * 3. Medir diferenças em CPU usage e execution time
 * 
 * Métricas reportadas:
 * • Tempo de execução com e sem profiler
 * • CPU overhead (%)
 * • Latência de sampling
 * • Context switches
 * • Memória utilizada
 */

void run_experiment_overhead();

#endif // EXPERIMENT_OVERHEAD_H
