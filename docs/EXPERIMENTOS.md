# Metodologia e Descrição dos Experimentos

## Visão Geral

Este documento descreve os **cinco experimentos práticos** implementados no Resource Monitor, detalhando a metodologia, objetivos, métricas coletadas e análise de resultados. Os experimentos foram projetados para demonstrar e validar os mecanismos fundamentais de **containerização Linux** (namespaces e cgroups v2).

---

## Experimento 1: Overhead de Monitoramento

### Objetivo
Medir o impacto (overhead) que o próprio sistema de monitoramento causa no processo sendo observado.

### Metodologia
1. **Carga de trabalho sintética**: Execução de operações de CPU, memória e I/O controladas
2. **Comparação**: Medir diferença de desempenho entre:
   - Processo executando **sem monitoramento**
   - Processo executando **com monitoramento ativo**
3. **Métricas coletadas**:
   - Tempo de execução total
   - Context switches (voluntários e involuntários)
   - Uso de CPU (user time e system time)

### Implementação
```c
// Arquivo: src/experiment_overhead.c
- Executa workload sem monitoramento (baseline)
- Executa workload com monitoramento (1s de intervalo)
- Calcula diferença percentual
```

### Métricas de Saída
- `execution_time` - Tempo total de execução (ms)
- `context_switches` - Número de trocas de contexto
- `overhead_percentage` - Percentual de overhead causado

### Resultado Esperado
Overhead típico: **< 5%** para intervalos de monitoramento de 1 segundo.

### Arquivo de Saída
`output/experiment1_overhead.csv`

---

## Experimento 2: Isolamento via Namespaces

### Objetivo
Validar o isolamento entre processos usando diferentes tipos de namespaces Linux e medir o overhead de criação.

### Metodologia
1. **Processo baseline**: Criar processo no namespace padrão do sistema
2. **Processos isolados**: Criar processos com diferentes combinações de namespaces:
   - PID namespace (isolamento de IDs de processo)
   - Network namespace (isolamento de rede)
   - Mount namespace (isolamento de sistema de arquivos)
   - UTS namespace (isolamento de hostname)
   - IPC namespace (isolamento de comunicação inter-processo)
3. **Validação**: Comparar inodes de namespaces entre processos
4. **Medição de overhead**: Tempo para criar cada tipo de namespace

### Implementação
```c
// Arquivo: src/experiments.c (função run_experiment_namespace_isolation)
- unshare() para criar namespaces
- readlink() em /proc/[pid]/ns/* para validar
- Medição de tempo com clock_gettime()
```

### Métricas de Saída
- `namespace_type` - Tipo de namespace testado
- `isolated` - Confirmação de isolamento (true/false)
- `creation_time_us` - Tempo de criação (microsegundos)
- `inode_parent` - Inode do namespace pai
- `inode_child` - Inode do namespace filho

### Resultado Esperado
- Isolamento confirmado: inodes diferentes entre pai e filho
- Overhead de criação: **< 1ms** por namespace

### Arquivo de Saída
`output/experiments/exp2_namespace_isolation.json`

---

## Experimento 3: CPU Throttling (Limitação de CPU)

### Objetivo
Demonstrar a eficácia de cgroups v2 em limitar o uso de CPU de um processo e validar a precisão do throttling.

### Metodologia
1. **Criar cgroup** específico para o experimento
2. **Configurar limite**: Definir limite de CPU usando `cpu.max`
   - Formato: `quota period` (ex: 50000 100000 = 50% de 1 CPU)
3. **Executar carga**: Processo intensivo de CPU (loop infinito)
4. **Coletar métricas**: Monitorar uso real de CPU vs limite configurado
5. **Duração**: 10 segundos de execução

### Implementação
```c
// Arquivo: src/experiment_cpu_throttling.c
- Cria /sys/fs/cgroup/resource_monitor_cpu_test
- Escreve limite em cpu.max
- Move processo para o cgroup via cgroup.procs
- Monitora cpu.stat (usage_usec, throttled_usec)
```

### Limites Testados
- 25% de 1 CPU (25000 / 100000)
- 50% de 1 CPU (50000 / 100000)
- 75% de 1 CPU (75000 / 100000)

### Métricas de Saída
- `cpu_limit_percent` - Limite configurado (%)
- `cpu_usage_actual` - Uso real observado (%)
- `throttled_time_ms` - Tempo que processo ficou throttled
- `accuracy` - Precisão do limite (diferença entre configurado e real)

### Resultado Esperado
- Precisão de throttling: **± 5%** do limite configurado
- Processo não excede limite definido

### Arquivo de Saída
`output/experiment3_cpu_throttling.csv`

---

## Experimento 4: Limite de Memória

### Objetivo
Validar mecanismos de limitação de memória via cgroups v2 e observar comportamento quando limite é excedido.

### Metodologia
1. **Criar cgroup** com limite de memória definido
2. **Configurar limite**: 100 MB via `memory.max`
3. **Tentativas de alocação**: Processos tentam alocar memória incrementalmente
   - Incremento: 10 MB por tentativa
   - Máximo de tentativas: 30 (total de 300 MB)
4. **Observar comportamento**:
   - Alocações bem-sucedidas (dentro do limite)
   - OOM killer ativado (fora do limite)
   - Memory events (eventos de pressão)

### Implementação
```c
// Arquivo: src/experiment_memory_limit.c
- Cria /sys/fs/cgroup/resource_monitor_mem_test
- Escreve "104857600" em memory.max (100 MB)
- Desabilita swap: memory.swap.max = 0
- Fork múltiplos processos alocando memória
- Monitora memory.events (oom, oom_kill)
```

### Métricas de Saída
- `attempt` - Número da tentativa de alocação
- `target_mb` - Memória alvo para alocar
- `allocated_mb` - Memória realmente alocada
- `status` - Sucesso ou falha (OOM killed)
- `memory_current` - Uso atual de memória
- `memory_peak` - Pico de uso durante experimento

### Resultado Esperado
- Alocações até 100 MB: **sucesso**
- Alocações > 100 MB: **falha (OOM kill)**
- Limite respeitado com precisão

### Arquivo de Saída
`output/experiment4_memory_limit.csv`

---

## Experimento 5: Limite de I/O (Disco)

### Objetivo
Demonstrar controle de operações de I/O (leitura/escrita) usando cgroups v2 e medir impacto no throughput.

### Metodologia
1. **Identificar dispositivo**: Obter major:minor do dispositivo de armazenamento
   - Ex: `/dev/sda` → `8:0`
2. **Criar cgroup** com limite de I/O
3. **Configurar limites** via `io.max`:
   - `rbps` (read bytes per second): Taxa de leitura
   - `wbps` (write bytes per second): Taxa de escrita
   - Ex: `8:0 wbps=1048576` = 1 MB/s
4. **Executar carga**: Operações de escrita intensivas com `dd`
5. **Medir throughput**: Bytes escritos / tempo decorrido

### Implementação
```c
// Arquivo: src/experiment_io_limit.c
- Identifica dispositivo raiz via stat("/")
- Cria /sys/fs/cgroup/resource_monitor_io_test
- Configura io.max com limites
- Executa dd if=/dev/zero of=/tmp/test bs=1M count=100
- Monitora io.stat (rbytes, wbytes)
```

### Limites Testados
- Sem limite (baseline)
- 1 MB/s
- 5 MB/s
- 10 MB/s

### Métricas de Saída
- `io_limit_mbps` - Limite configurado (MB/s)
- `io_actual_mbps` - Throughput real medido (MB/s)
- `bytes_written` - Total de bytes escritos
- `duration_ms` - Duração da operação
- `efficiency` - Razão entre limite e throughput real

### Resultado Esperado
- Throughput não excede limite configurado
- Precisão: **± 10%** do limite (devido a buffers do kernel)

### Arquivo de Saída
`output/experiment5_io_limit.csv`

---

## Execução dos Experimentos

### Via Menu Interativo
```bash
sudo ./bin/monitor menu
# Escolher opção 4 (Experimentos)
# Selecionar experimento individual ou executar todos
```

### Via Linha de Comando
```bash
# Individual
./bin/monitor experiment 1              # Não requer root
sudo ./bin/monitor experiment 2         # Requer root
sudo ./bin/monitor experiment 3         # Requer root
sudo ./bin/monitor experiment 4         # Requer root
sudo ./bin/monitor experiment 5         # Requer root

# Todos de uma vez
sudo ./bin/monitor menu
# Opção 4 → Opção 6 (Executar TODOS)
```

---

## Visualização de Resultados

### Geração Automática de Gráficos
```bash
# Via menu
sudo ./bin/monitor menu
# Opção 4 → Opção 7 (Gerar visualizações)

# Via script Python
source venv/bin/activate
python scripts/visualize.py --experiments output/graphs
```

### Gráficos Gerados
- `exp1_overhead.png` - Comparação de overhead
- `exp1_context_switches.png` - Context switches
- `exp1_execution_time.png` - Tempo de execução
- `exp3_cpu_usage.png` - Uso de CPU vs limite
- `exp4_memory_usage.png` - Alocação de memória
- `exp5_io_operations.png` - Throughput de I/O

---

## Análise de Resultados

### Critérios de Validação

**Experimento 1 - Overhead:**
- ✅ Overhead < 5%: Monitoramento eficiente
- ⚠️ Overhead 5-10%: Aceitável para intervalos curtos
- ❌ Overhead > 10%: Requer otimização

**Experimento 2 - Namespaces:**
- ✅ Inodes diferentes: Isolamento confirmado
- ✅ Tempo < 1ms: Overhead aceitável
- ❌ Inodes iguais: Isolamento falhou

**Experimento 3 - CPU Throttling:**
- ✅ |real - limite| < 5%: Throttling preciso
- ⚠️ |real - limite| 5-10%: Imprecisão tolerável
- ❌ |real - limite| > 10%: Configuração incorreta

**Experimento 4 - Memória:**
- ✅ Alocações <= limite: Sucesso
- ✅ Alocações > limite: OOM kill
- ❌ Excedeu limite sem OOM: Falha do cgroup

**Experimento 5 - I/O:**
- ✅ Throughput <= limite: Controle efetivo
- ⚠️ Throughput até 110% do limite: Buffers do kernel
- ❌ Throughput >> limite: Configuração incorreta

---

## Requisitos Técnicos

### Sistema Operacional
- Linux kernel 4.5+ (cgroups v2 unificado)
- Systemd com cgroups v2 habilitado

### Verificação de Suporte
```bash
# Verificar cgroups v2
mount | grep cgroup2

# Verificar controllers
cat /sys/fs/cgroup/cgroup.controllers
# Esperado: cpu memory io pids
```

### Permissões
- Experimento 1: **Não requer root**
- Experimentos 2-5: **Requer root** (criação de cgroups e namespaces)

---

## Referências

- [Linux Kernel - Control Groups v2](https://docs.kernel.org/admin-guide/cgroup-v2.html)
- [Linux Kernel - Namespaces](https://man7.org/linux/man-pages/man7/namespaces.7.html)
- [cgroups(7) - Linux manual page](https://man7.org/linux/man-pages/man7/cgroups.7.html)
- Documentação do projeto: `docs/ARCHITECTURE.md`
