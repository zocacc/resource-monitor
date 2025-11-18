# Experimento 1: Overhead de Monitoramento

## Objetivo
Medir o impacto do próprio profiler no sistema

## Procedimento

### 1. Executar workload de referência SEM monitoramento
- Workload: Loop de CPU intensivo com 1.000.000 iterações
- Comando: `for i in {1..1000000}; do result=$((i*i*i)); done`
- Medir tempo de execução total (baseline)

### 2. Executar mesmo workload COM monitoramento - Intervalo 1s
- Mesmo workload de referência
- Monitoramento ativo com sampling a cada 1 segundo
- Coletar métricas: CPU usage, Memory usage
- Medir tempo de execução total
- Contar número de amostras coletadas
- Calcular latência média de sampling

### 3. Executar mesmo workload COM monitoramento - Intervalo 100ms
- Mesmo workload de referência
- Monitoramento ativo com sampling a cada 100 milissegundos
- Coletar métricas: CPU usage, Memory usage
- Medir tempo de execução total
- Contar número de amostras coletadas
- Calcular latência média de sampling

### 4. Calcular diferenças e overhead
- Comparar tempos de execução
- Calcular overhead percentual para cada intervalo
- Analisar impacto da frequência de sampling

## Métricas Reportadas (JSON)

### Estrutura do Relatório

```json
{
  "experiment": "Overhead de Monitoramento",
  "objective": "Medir o impacto do próprio profiler no sistema",
  "date": "ISO 8601 timestamp",
  
  "workload": {
    "description": "Descrição do workload",
    "command": "Comando executado"
  },
  
  "baseline": {
    "description": "Execução sem monitoramento",
    "execution_time_ms": <tempo em milissegundos>,
    "execution_time_s": <tempo em segundos>
  },
  
  "monitored_1s_interval": {
    "description": "Execução com monitoramento (sampling a cada 1 segundo)",
    "execution_time_ms": <tempo total>,
    "execution_time_s": <tempo total em segundos>,
    "samples_collected": <número de amostras>,
    "avg_sampling_latency_us": <latência média em microsegundos>,
    "overhead_ms": <diferença em relação ao baseline>,
    "overhead_percentage": <percentual de overhead>
  },
  
  "monitored_100ms_interval": {
    "description": "Execução com monitoramento (sampling a cada 100ms)",
    "execution_time_ms": <tempo total>,
    "execution_time_s": <tempo total em segundos>,
    "samples_collected": <número de amostras>,
    "avg_sampling_latency_us": <latência média em microsegundos>,
    "overhead_ms": <diferença em relação ao baseline>,
    "overhead_percentage": <percentual de overhead>
  },
  
  "metrics_summary": {
    "baseline_execution_time_ms": <baseline>,
    "monitoring_1s_execution_time_ms": <tempo com monitoramento 1s>,
    "monitoring_100ms_execution_time_ms": <tempo com monitoramento 100ms>,
    "cpu_overhead_1s_percentage": <overhead % para 1s>,
    "cpu_overhead_100ms_percentage": <overhead % para 100ms>,
    "sampling_latency_1s_us": <latência 1s>,
    "sampling_latency_100ms_us": <latência 100ms>
  },
  
  "conclusions": [
    "Conclusão 1 sobre overhead com intervalo 1s",
    "Conclusão 2 sobre overhead com intervalo 100ms",
    "Conclusão 3 sobre trade-off overhead vs granularidade",
    "Conclusão 4 sobre latências de sampling"
  ]
}
```

## Métricas Principais

### 1. Tempo de execução com e sem profiler
- **Baseline (ms)**: Tempo sem monitoramento
- **Monitoring 1s (ms)**: Tempo com sampling a cada 1s
- **Monitoring 100ms (ms)**: Tempo com sampling a cada 100ms

### 2. CPU Overhead (%)
- **Overhead 1s**: Percentual de aumento em relação ao baseline
- **Overhead 100ms**: Percentual de aumento em relação ao baseline
- Fórmula: `((monitored_time - baseline_time) / baseline_time) * 100`

### 3. Latência de Sampling
- **Latência 1s (μs)**: Tempo médio para coletar uma amostra (intervalo 1s)
- **Latência 100ms (μs)**: Tempo médio para coletar uma amostra (intervalo 100ms)
- Calculado como: `total_sampling_time / number_of_samples`

### 4. Amostras Coletadas
- Número total de amostras coletadas durante a execução
- Maior em intervalos menores (100ms vs 1s)

## Resultados Esperados

### Baseline
- Tempo de execução puro do workload sem interferência
- Referência para cálculo de overhead

### Com Monitoramento (1s)
- Overhead esperado: **~1-3%**
- Latência baixa devido a menor frequência
- Menos amostras coletadas
- Adequado para monitoramento de longa duração

### Com Monitoramento (100ms)
- Overhead esperado: **~3-8%**
- Latência ligeiramente maior devido a maior frequência
- Mais amostras coletadas (10x mais que 1s)
- Adequado para análise detalhada de curto prazo

## Conclusões Típicas

1. **Trade-off overhead vs granularidade**
   - Intervalos menores = maior overhead
   - Intervalos menores = mais dados para análise

2. **Impacto aceitável**
   - Overhead abaixo de 10% é considerado aceitável
   - Permite monitoramento em produção

3. **Latência de sampling**
   - Operação de coleta é rápida (< 1ms)
   - Não bloqueia processo monitorado significativamente

## Arquivo de Saída

**Localização**: `output/experiments/exp1_monitoring_overhead.json`

**Formato**: JSON formatado com indentação

**Visualização**: Exibido automaticamente com `jq` se disponível
