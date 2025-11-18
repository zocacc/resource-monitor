# Guia de Visualização de Experimentos

## Visão Geral

O sistema de visualização gera gráficos automaticamente para todos os 5 experimentos do Resource Monitor.

## Funcionalidades

### 1. Geração Automática via Menu
Ao executar um experimento via menu interativo, os gráficos são gerados automaticamente.

```bash
sudo ./bin/monitor
# Selecione opção 4 (Experimentos)
# Escolha o experimento desejado (1-5)
# Os gráficos serão gerados automaticamente em output/graphs/
```

### 2. Geração Manual

#### Experimento Individual
```bash
# Experimento 1
python3 scripts/visualize.py output/experiment1_overhead.csv output/graphs

# Experimento 2
python3 scripts/visualize.py output/experiments/exp2_namespace_isolation.json output/graphs

# Experimento 3
python3 scripts/visualize.py output/experiment3_cpu_throttling.csv output/graphs

# Experimento 4
python3 scripts/visualize.py output/experiment4_memory_limit.csv output/graphs

# Experimento 5
python3 scripts/visualize.py output/experiment5_io_limit.csv output/graphs
```

#### Todos os Experimentos
```bash
python3 scripts/visualize.py --experiments output/graphs
```

## Gráficos Gerados por Experimento

### Experimento 1: Overhead de Monitoramento
- `exp1_overhead.png` - Overhead de tempo e CPU vs intervalo de sampling
- `exp1_context_switches.png` - Context switches adicionais por intervalo
- `exp1_execution_time.png` - Tempo de execução por configuração

### Experimento 2: Isolamento via Namespaces
- `exp2_creation_time.png` - Tempo de criação de namespaces
- `exp2_resource_visibility.png` - Recursos visíveis: Host vs Namespace
- `exp2_isolation_effectiveness.png` - Efetividade do isolamento (pizza chart)

### Experimento 3: CPU Throttling
- `exp3_cpu_comparison.png` - CPU medido vs limite configurado
- `exp3_deviation.png` - Desvio percentual por limite
- `exp3_throughput.png` - Throughput vs limite de CPU

### Experimento 4: Limitação de Memória
- `exp4_allocation_progress.png` - Progresso de alocação de memória
- `exp4_allocation_status.png` - Status de alocação por passo
- `exp4_current_vs_peak.png` - Uso atual vs pico de memória

### Experimento 5: Limitação de I/O
- `exp5_write_throughput.png` - Throughput de escrita vs limite
- `exp5_read_throughput.png` - Throughput de leitura vs limite
- `exp5_latency.png` - Latências de escrita e leitura
- `exp5_time_impact.png` - Impacto no tempo total de execução

## Requisitos

### Dependências Python
```bash
pip3 install matplotlib numpy
```

### Permissões
```bash
# Criar diretório de saída com permissões adequadas
sudo mkdir -p output/graphs
sudo chmod 777 output/graphs
```

## Estrutura de Arquivos

```
resource-monitor/
├── scripts/
│   └── visualize.py          # Script de visualização
├── output/
│   ├── graphs/               # Gráficos gerados (PNG)
│   ├── experiments/          # Dados JSON dos experimentos
│   ├── experiment1_overhead.csv
│   ├── experiment3_cpu_throttling.csv
│   ├── experiment4_memory_limit.csv
│   └── experiment5_io_limit.csv
```

## Formato de Dados

### Experimento 1 (CSV)
Colunas: sampling_interval_ms, execution_time_sec, cpu_time_sec, context_switches, time_overhead_percent, cpu_overhead_percent, ctx_switches_delta

### Experimento 2 (JSON)
Estrutura:
```json
{
  "experiment": "Namespace Isolation",
  "timestamp": "...",
  "tests": [
    {
      "namespace_type": "pid",
      "creation_time_us": 1234,
      "isolated": true,
      "host_resources": 150,
      "namespace_resources": 5
    }
  ]
}
```

### Experimentos 3, 4, 5 (CSV)
Ver cabeçalhos nos respectivos arquivos CSV.

## Troubleshooting

### Erro: "matplotlib ou numpy não instalado"
```bash
pip3 install matplotlib numpy
```

### Erro: "Permission denied"
```bash
sudo mkdir -p output/graphs
sudo chmod 777 output/graphs
```

### Arquivos não encontrados
Execute os experimentos primeiro:
```bash
sudo ./bin/monitor experiment 1
sudo ./bin/monitor experiment 2
# ... etc
```

## Integração com main.c

O arquivo `src/main.c` foi modificado para chamar automaticamente `visualize.py` após a execução de cada experimento:

```c
// Após executar experimento
char vis_cmd[512];
snprintf(vis_cmd, sizeof(vis_cmd), 
        "python3 scripts/visualize.py %s output/graphs 2>&1 | tail -20", 
        exp_file);
system(vis_cmd);
```

## Características Técnicas

- **Resolução**: 150 DPI (alta qualidade para relatórios)
- **Formato**: PNG com compressão
- **Cores**: Paleta otimizada para visualização científica
- **Gráficos**: Matplotlib com estilo profissional
- **Anotações**: Valores exibidos diretamente nos gráficos
- **Grid**: Habilitado para facilitar leitura
- **Legendas**: Descritivas e claras

## Emojis Removidos

Todos os emojis foram removidos do output do terminal para melhor compatibilidade com diferentes terminais e sistemas de log.

## Exemplo de Uso Completo

```bash
# 1. Executar experimento
sudo ./bin/monitor experiment 1

# 2. Verificar gráficos
ls output/graphs/exp1_*.png

# 3. Visualizar (pode usar qualquer visualizador de imagens)
eog output/graphs/exp1_overhead.png         # Eye of GNOME
feh output/graphs/exp1_overhead.png         # feh
xdg-open output/graphs/exp1_overhead.png    # Visualizador padrão
```

## Notas

- Os gráficos são regenerados a cada execução (sobrescrevem arquivos anteriores)
- Cada experimento gera de 3 a 4 gráficos diferentes
- O modo `--experiments` gera todos os gráficos de uma vez (se os dados existirem)
- A geração é rápida (< 1 segundo por conjunto de gráficos)
