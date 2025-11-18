#ifndef EXPERIMENTS_H
#define EXPERIMENTS_H

// Experimento 1: Overhead de Monitoramento
// Mede o impacto do próprio profiler no sistema
void run_experiment_monitoring_overhead(const char *output_file);

// Experimento 2: Isolamento via Namespaces
// Valida efetividade do isolamento de namespaces
void run_experiment_namespace_isolation(const char *output_file);

// Experimento 3: CPU Throttling com Cgroups
// Demonstra limitação de CPU usando cgroups v2
void run_experiment_cpu_throttling(const char *output_file);

// Experimento 4: Limite de Memória com Cgroups
// Demonstra limitação de memória usando cgroups v2
void run_experiment_memory_limit(const char *output_file);

// Experimento 5: Limite de I/O com Cgroups
// Demonstra limitação de I/O usando cgroups v2
void run_experiment_io_limit(const char *output_file);

#endif // EXPERIMENTS_H
