# Resource Monitor - Guia de Uso Rápido (Versão Refatorada)

## 🚀 Início Rápido

### 1. Compilar o Projeto
```bash
cd resource-monitor
make clean
make
```

### 2. Executar Demo Completo
```bash
./scripts/demo_presentation_v2.sh
```

---

## 📋 Comandos Disponíveis

### Monitoramento de Processos

#### TUI Interativo (pressione 'q' para sair)
```bash
./bin/monitor tui <PID>
```

#### TUI com Tempo Determinado
```bash
./bin/monitor tui <PID> <intervalo_s> <duração_s>

# Exemplo: Monitorar PID 1234 a cada 2s por 60s
./bin/monitor tui 1234 2 60
```

#### Monitoramento CLI (sem interface)
```bash
./bin/monitor monitor <PID> <intervalo_s> <duração_s>

# Exemplo: Coletar dados do PID 5678 a cada 1s por 30s
./bin/monitor monitor 5678 1 30
# Saída: output/monitor_output.json
```

---

### Análise de Namespaces

#### Listar namespaces de um processo
```bash
./bin/monitor namespace list <PID>
```

#### Comparar namespaces de dois processos
```bash
./bin/monitor namespace compare <PID1> <PID2>
```

#### Encontrar processos em um namespace
```bash
./bin/monitor namespace find /proc/<PID>/ns/<tipo>

# Exemplo:
./bin/monitor namespace find /proc/1/ns/pid
```

#### Relatório completo do sistema (requer root)
```bash
sudo ./bin/monitor namespace report
# Saída: output/namespace_report.json
```

#### Medir overhead de criação de namespace
```bash
./bin/monitor namespace overhead
```

---

### Gerenciamento de Cgroups (requer root)

```bash
# Listar todos os cgroups
sudo ./bin/cgroup_manager list

# Criar novo cgroup
sudo ./bin/cgroup_manager create <nome>

# Mover processo para cgroup
sudo ./bin/cgroup_manager move <cgroup> <PID>

# Configurar limites
sudo ./bin/cgroup_manager limits <cgroup>

# Ver métricas
sudo ./bin/cgroup_manager report <cgroup>
```

---

### Experimentos (NOVO!)

#### Executar experimento específico
```bash
# Experimento 1: Overhead de Monitoramento (implementado em C)
./bin/monitor experiment 1

# Experimentos 2-5: Via script shell
sudo ./scripts/demo_presentation_v2.sh
# Escolher opção 4 → Experimento desejado
```

#### Ver resultados dos experimentos
```bash
# Listar arquivos JSON gerados
ls -lh output/experiments/

# Visualizar com jq (se instalado)
cat output/experiments/exp1_monitoring_overhead.json | jq .
```

---

## 📊 Experimentos Disponíveis

### 1. Overhead de Monitoramento ✅ (Implementado em C)
**Objetivo**: Medir impacto do profiler no sistema

**Como executar**:
```bash
./bin/monitor experiment 1
```

**Saída**: 
- `output/experiments/exp1_monitoring_overhead.json`
- Métricas: overhead %, latência de sampling, amostras coletadas

**Métricas**:
- Baseline (sem monitoramento)
- Com monitoramento (intervalos 1s e 100ms)
- Overhead percentual
- Latência de sampling em μs

---

### 2. Isolamento via Namespaces
**Objetivo**: Validar efetividade do isolamento

**Como executar**:
```bash
sudo ./scripts/demo_presentation.sh.backup
# Opção 4 → 2
```

**Testa**:
- PID, NET, UTS, IPC, MOUNT namespaces
- Isolamento de processos, rede, hostname, IPC
- Overhead de criação

---

### 3. CPU Throttling com Cgroups
**Objetivo**: Limitar uso de CPU

**Testa**:
- Criação de cgroup com limite de 20% CPU
- Workload CPU intensivo
- Monitoramento de throttling

---

### 4. Limite de Memória com Cgroups
**Objetivo**: Limitar uso de memória

**Testa**:
- Limite de 50MB
- Alocação gradual de memória
- Comportamento ao atingir limite

---

### 5. Limite de I/O com Cgroups
**Objetivo**: Limitar operações de disco

**Testa**:
- Limite de throughput de I/O
- Operações de leitura/escrita
- Impacto no desempenho

---

## 🎨 Visualizações

### Gerar gráficos (requer Python + matplotlib)
```bash
# Primeiro, coletar dados com TUI
./bin/monitor tui <PID> 1 60

# Depois, gerar gráficos
python3 scripts/visualize.py
```

**Gráficos gerados**:
- `output/cpu_usage.png` - Uso de CPU ao longo do tempo
- `output/memory_usage.png` - VSZ e RSS
- `output/io_rates.png` - Taxas de leitura/escrita

---

## 🔍 Análise de Memória com Valgrind

```bash
./scripts/valgrind_analysis.sh
```

**Verifica**:
- Memory leaks
- Invalid reads/writes
- Uso de memória não inicializada

---

## 📁 Estrutura de Saída

```
output/
├── monitor_output.json           # Dados de monitoramento TUI/CLI
├── namespace_report.json         # Relatório de namespaces
└── experiments/
    ├── exp1_monitoring_overhead.json
    ├── exp2_namespace_isolation.json
    ├── exp3_cpu_throttling.json
    ├── exp4_memory_limit.json
    └── exp5_io_limit.json
```

---

## 🛠️ Troubleshooting

### Erro: "Permission denied" ao criar JSON
```bash
# Remover diretório com permissões incorretas
sudo rm -rf output/experiments
mkdir -p output/experiments
```

### Erro: "bin/monitor not found"
```bash
make clean
make
```

### Experimentos requerem root
```bash
# Executar com sudo
sudo ./bin/monitor experiment <N>

# Ou usar o script que solicita permissão automaticamente
sudo ./scripts/demo_presentation_v2.sh
```

### Namespace report retorna 100% compartilhado
```bash
# Execute com sudo
sudo ./bin/monitor namespace report
```

---

## 📖 Documentação Adicional

- **Arquitetura**: `docs/ARCHITECTURE.md`
- **Guia TUI**: `docs/TUI_GUIDE.md`
- **Comandos Cgroup**: `docs/CGROUP_COMMANDS.md`
- **Refatoração**: `docs/REFACTORING_SUMMARY.md` ⭐ NOVO

---

## 🎯 Exemplos Práticos

### Monitorar servidor web
```bash
# Encontrar PID do nginx
ps aux | grep nginx

# Monitorar por 5 minutos
./bin/monitor tui 12345 2 300

# Gerar gráficos
python3 scripts/visualize.py
```

### Testar isolamento de containers
```bash
# Criar cgroup
sudo ./bin/cgroup_manager create mycontainer

# Limitar CPU a 50%
echo "50000 100000" | sudo tee /sys/fs/cgroup/mycontainer/cpu.max

# Mover processo
sudo ./bin/cgroup_manager move mycontainer <PID>

# Monitorar
sudo ./bin/cgroup_manager report mycontainer
```

### Experimento completo
```bash
# Executar todos os experimentos
./scripts/demo_presentation_v2.sh
# Opção 4 → 6 (Executar TODOS)

# Ver resultados
ls output/experiments/
cat output/experiments/exp1_monitoring_overhead.json | jq .
```

---

## 🔗 Links Úteis

- **cgroups v2**: https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html
- **namespaces**: https://man7.org/linux/man-pages/man7/namespaces.7.html
- **/proc filesystem**: https://man7.org/linux/man-pages/man5/proc.5.html

---

## 👥 Contribuidores

Grupo 15 - Sistemas Operacionais - RA3

---

**Versão Refatorada**: Lógica em C, apresentação em Shell
**Última atualização**: Novembro 2024
