#ifndef EXPERIMENT_MEMORY_LIMIT_H
#define EXPERIMENT_MEMORY_LIMIT_H

/**
 * Experimento 4: Limitação de Memória
 * 
 * Objetivo: Testar comportamento ao atingir limite de memória
 * 
 * Procedimento:
 * 1. Criar cgroup com limite de 100MB
 * 2. Tentar alocar memória incrementalmente
 * 3. Observar comportamento (OOM killer, falhas de alocação)
 * 
 * Métricas reportadas:
 * • Quantidade máxima alocada
 * • Número de falhas (memory.failcnt / memory.events)
 * • Comportamento do sistema ao atingir limite
 */

void run_experiment_memory_limit_new();

#endif // EXPERIMENT_MEMORY_LIMIT_H
