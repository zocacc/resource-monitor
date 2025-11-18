#ifndef EXPERIMENT_CPU_THROTTLING_H
#define EXPERIMENT_CPU_THROTTLING_H

/**
 * Experimento 3: Throttling de CPU
 * 
 * Objetivo: Avaliar precisão de limitação de CPU via cgroups
 * 
 * Procedimento:
 * 1. Executar processo CPU-intensive sem limite
 * 2. Aplicar limites de 0.25, 0.5, 1.0 e 2.0 cores
 * 3. Medir CPU usage real em cada configuração
 * 
 * Métricas reportadas:
 * • CPU% medido vs limite configurado
 * • Desvio percentual
 * • Throughput (iterações/segundo) em cada configuração
 */

void run_experiment_cpu_throttling_new();

#endif // EXPERIMENT_CPU_THROTTLING_H
