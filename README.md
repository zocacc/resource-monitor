# Resource Monitor - Containers e Recursos

Sistema completo de monitoramento de processos Linux com suporte a namespaces, cgroups v2 e experimentos de isolamento e limitação de recursos. Desenvolvido como trabalho acadêmico (RA3) para a disciplina de Sistemas Operacionais.

**Autor:** Enzo Capellari - Grupo 9

## 📖 Descrição do Projeto

Este projeto implementa um **profiler de recursos de sistema** em C que permite:

1. **Monitoramento em tempo real** de processos Linux (CPU, memória, I/O, rede)
2. **Análise de namespaces** para validação de isolamento entre processos
3. **Gerenciamento de cgroups v2** para limitação e controle de recursos
4. **Cinco experimentos práticos** demonstrando conceitos de containerização
5. **Interface TUI (Text User Interface)** com visualização gráfica de métricas
6. **Visualizações gráficas** geradas automaticamente com matplotlib

O sistema foi desenvolvido para demonstrar na prática os mecanismos fundamentais de **containers Linux** (namespaces e cgroups) e medir o overhead de diferentes técnicas de isolamento e monitoramento.

## 📁 Estrutura do Projeto

| Módulo | Arquivo(s) Principal(is) | Descrição |
|--------|-------------------------|-----------|
| **Core do Monitor** | `src/main.c`, `src/monitor_tui.c` | Menu interativo, interface TUI, loop de monitoramento |
| **Coleta de Métricas** | `src/cpu_monitor.c`, `src/memory_monitor.c`, `src/io_monitor.c`, `src/network_monitor.c` | Leitura de dados do `/proc` e cálculos de uso |
| **Namespace Analyzer** | `src/namespace_analyzer.c` | Análise, comparação e relatórios de namespaces |
| **Cgroup Manager** | `src/cgroup_v2.c`, `src/cgroup_manager.c` | Gerenciamento de cgroups v2, aplicação de limites |
| **Experimento 1** | `src/experiment_overhead.c` | Medição de overhead de monitoramento |
| **Experimento 2** | `src/experiments.c` (namespace) | Validação de isolamento via namespaces |
| **Experimento 3** | `src/experiment_cpu_throttling.c` | Demonstração de CPU throttling |
| **Experimento 4** | `src/experiment_memory_limit.c` | Demonstração de limites de memória |
| **Experimento 5** | `src/experiment_io_limit.c` | Demonstração de limites de I/O |
| **Visualização** | `scripts/visualize.py` | Geração de gráficos com matplotlib |
| **Testes Unitários** | `tests/*.c` | 5 suítes de teste para validação |
| **Utilitários** | `src/utils.c`, `src/process_monitor.c` | Funções auxiliares e exportação de dados |
| **Documentação** | `docs/*.md`, `README.md` | Documentação técnica e guias |
| **Build System** | `Makefile`, `build.sh` | Sistema de compilação e scripts de build |

## 🔧 Requisitos e Dependências

### Requisitos de Sistema

- **Sistema Operacional:** Linux (kernel 4.5+)
- **Arquitetura:** x86_64
- **Cgroups v2:** Habilitado no kernel
- **Privilégios:** Root necessário para experimentos 2-5 e gerenciamento de cgroups

### Dependências Obrigatórias

```bash
# Arch Linux
sudo pacman -S gcc make ncurses util-linux iproute2 coreutils

# Ubuntu/Debian
sudo apt-get install gcc make libncurses-dev util-linux iproute2 coreutils
```

**Pacotes:**
- `gcc` - Compilador C (GCC 9.0+)
- `make` - Sistema de build
- `ncurses` - Biblioteca para interface TUI
- `util-linux` - Ferramentas (unshare, nsenter)
- `iproute2` - Ferramentas de rede (ip)
- `coreutils` - Utilitários GNU (dd, cat, etc.)

### Dependências Opcionais (Visualização)

Para gerar gráficos automaticamente:

```bash
# Criar ambiente virtual Python
python3 -m venv venv
source venv/bin/activate  # ou ./venv/bin/activate

# Instalar dependências Python
pip install matplotlib numpy
```

### Verificar Suporte a Cgroups v2

```bash
# Verificar se cgroups v2 está montado
mount | grep cgroup2
# Saída esperada: cgroup2 on /sys/fs/cgroup type cgroup2 (rw,...)

# Verificar controllers disponíveis
cat /sys/fs/cgroup/cgroup.controllers
# Saída esperada: cpu memory io pids ...
```

## 🛠️ Instruções de Compilação

### Método 1: Build Rápido (Recomendado)

```bash
./build.sh
```

Este script:
1. Compila todo o código fonte
2. Cria diretórios de saída
3. Executa automaticamente o menu interativo

### Método 2: Build Manual

```bash
# Compilação limpa
make clean && make

# Apenas compilar (sem limpar)
make

# Limpar arquivos de build
make clean
```

**Saída da compilação:**
- `bin/monitor` - Binário principal (todas as funcionalidades)
- `bin/cgroup_manager` - Utilitário de gerenciamento de cgroups
- `obj/*.o` - Arquivos objeto intermediários

### Verificar Compilação

```bash
# Verificar binários criados
ls -lh bin/

# Testar execução
./bin/monitor --help
```

## 📚 Instruções de Uso

### Menu Interativo (Modo Recomendado)

```bash
./bin/monitor menu
```

**Menu principal oferece:**
1. Resource Monitor (TUI em tempo real)
2. Namespace Analyzer (análise de isolamento)
3. Control Group Manager (gerenciamento de cgroups)
4. Experimentos (1-5 + geração de visualizações)

### Interface TUI (Text User Interface)

O Resource Monitor possui uma interface TUI rica em recursos construída com ncurses:

**Recursos da TUI:**
- 📊 Visualização em tempo real de CPU, Memória, I/O e Rede
- 📈 Gráficos de barras coloridos para métricas
- 📉 Histórico de 60 segundos com sparklines
- ⚡ Atualização automática a cada 1 segundo
- 🎨 Código de cores: Verde (normal), Amarelo (médio), Vermelho (alto)
- 🔄 Suporte a redimensionamento de terminal

**Controles:**
- `q` - Sair
- `h` - Ajuda
- `r` - Atualizar manualmente
- `p` - Pausar/Continuar

**Executar TUI:**
```bash
# Modo interativo (rodando até pressionar 'q')
./bin/monitor tui <PID>

# Modo com tempo determinado
./bin/monitor tui <PID> <intervalo_s> <duração_s>

# Exemplo: monitorar processo 1234
./bin/monitor tui 1234
```

### Modo Linha de Comando

#### Monitoramento de Processos

```bash
# TUI - Interface em tempo real (pressione 'q' para sair)
./bin/monitor tui <PID>

# Exemplo: monitorar processo 1234
./bin/monitor tui 1234

# TUI com tempo determinado (5s de intervalo por 60s)
./bin/monitor tui 1234 5 60

# Monitoramento com exportação JSON
./bin/monitor process 1234 5 60 json

# Monitoramento com exportação CSV
./bin/monitor process 1234 5 60 csv
```

**Exemplo prático - Monitorar navegador:**
```bash
# Encontrar PID do Firefox
pgrep firefox

# Monitorar em tempo real
./bin/monitor tui $(pgrep firefox | head -1)
```

#### Análise de Namespaces

```bash
# Listar namespaces de um processo
./bin/monitor namespace list <PID>

# Comparar namespaces entre dois processos
./bin/monitor namespace compare <PID1> <PID2>

# Encontrar processos em um namespace específico
./bin/monitor namespace find /proc/1/ns/pid

# Gerar relatório completo do sistema
./bin/monitor namespace report

# Medir overhead de criação de namespaces
./bin/monitor namespace overhead
```

**Exemplo prático:**
```bash
# Comparar processo normal com processo em container
./bin/monitor namespace compare 1 $(pgrep dockerd)
```

#### Gerenciamento de Cgroups

```bash
# Modo interativo (requer root)
sudo ./bin/cgroup_manager

# Ou através do menu principal
sudo ./bin/monitor menu
# Escolha opção 3 (Control Group Manager)
```

#### Execução de Experimentos

```bash
# Experimento 1: Overhead de Monitoramento (sem root)
./bin/monitor experiment 1

# Experimento 2: Isolamento via Namespaces (requer root)
sudo ./bin/monitor experiment 2

# Experimento 3: CPU Throttling (requer root)
sudo ./bin/monitor experiment 3

# Experimento 4: Limite de Memória (requer root)
sudo ./bin/monitor experiment 4

# Experimento 5: Limite de I/O (requer root)
sudo ./bin/monitor experiment 5
```

### Exemplos de Uso Completos

#### Exemplo 1: Monitorar Servidor Web

```bash
# Iniciar servidor (exemplo)
python3 -m http.server 8000 &

# Obter PID
PID=$(pgrep -f "http.server")

# Monitorar em tempo real
./bin/monitor tui $PID

# Ou exportar métricas para análise
./bin/monitor process $PID 2 120 json
# Saída: output/process_monitoring.json
```

#### Exemplo 2: Validar Isolamento de Container

```bash
# Comparar namespaces do sistema vs container Docker
sudo ./bin/monitor namespace compare 1 $(docker inspect -f '{{.State.Pid}}' <container_name>)
```

#### Exemplo 3: Limitar CPU de Processo

```bash
# Via menu interativo
sudo ./bin/monitor menu
# 1. Escolha opção 3 (Control Groups)
# 2. Escolha opção 4 (Criar cgroup)
# 3. Escolha opção 6 (Mover processo)
# 4. Escolha opção 7 (Aplicar limites)

# Ou execute o experimento 3 que demonstra isso
sudo ./bin/monitor experiment 3
```

#### Exemplo 4: Executar Todos os Experimentos e Gerar Visualizações

```bash
# Via menu
sudo ./bin/monitor menu
# Escolha opção 4 (Experimentos)
# Escolha opção 6 (Executar TODOS)
# Aguarde conclusão...
# Escolha opção 7 (Gerar visualizações)

# Visualizar gráficos gerados
ls -lh output/graphs/
# exp1_overhead.png
# exp1_context_switches.png
# exp1_execution_time.png
# exp3_cpu_usage.png
# exp4_memory_usage.png
# exp5_io_operations.png
```

#### Exemplo 5: Gerar Visualizações Manualmente

```bash
# Ativar ambiente virtual Python
source venv/bin/activate

# Gerar visualizações de todos os experimentos
venv/bin/python scripts/visualize.py --experiments output/graphs

# Gerar visualização de experimento específico
venv/bin/python scripts/visualize.py output/experiment1_overhead.csv output/graphs
```

### Estrutura de Saída

```
output/
├── experiment1_overhead.csv              # Dados do experimento 1
├── experiment3_cpu_throttling.csv        # Dados do experimento 3
├── experiment4_memory_limit.csv          # Dados do experimento 4
├── experiment5_io_limit.csv              # Dados do experimento 5
├── experiments/
│   └── exp2_namespace_isolation.json     # Dados do experimento 2
├── graphs/
│   ├── exp1_overhead.png                 # Gráficos gerados
│   ├── exp1_context_switches.png
│   ├── exp1_execution_time.png
│   ├── exp3_cpu_usage.png
│   ├── exp4_memory_usage.png
│   └── exp5_io_operations.png
└── process_monitoring.json               # Dados de monitoramento contínuo
```

## 🧪 Testes e Validação

### Testes Unitários

O projeto inclui testes unitários para validar os componentes principais:

**Testes disponíveis:**
- `test_cpu.c` - Testa coleta de métricas de CPU
- `test_memory.c` - Testa coleta de métricas de memória
- `test_io.c` - Testa coleta de métricas de I/O
- `test_namespace.c` - Testa análise de namespaces
- `test_cgroup.c` - Testa funcionalidade de cgroups v2

**Compilar e executar testes:**

```bash
# Compilar testes
make tests

# Executar todos os testes (requer root)
make run_tests

# Ou executar individualmente
sudo ./bin/tests/test_cpu
sudo ./bin/tests/test_memory
sudo ./bin/tests/test_io
sudo ./bin/tests/test_namespace
sudo ./bin/tests/test_cgroup

# Executar teste específico sem root (funcionalidade limitada)
./bin/tests/test_cpu          # Não requer root
./bin/tests/test_cgroup       # Alguns testes pulados sem root
```

**Detalhes de cada teste:**

1. **test_cpu.c** - Valida coleta de métricas de CPU
   - ✅ Leitura de /proc/[pid]/stat
   - ✅ Parsing de campos (utime, stime, etc.)
   - ✅ Cálculo de uso de CPU
   - Não requer root

2. **test_memory.c** - Valida coleta de métricas de memória
   - ✅ Leitura de /proc/[pid]/status
   - ✅ Parsing de VmSize, VmRSS, VmData
   - ✅ Conversão de unidades (kB → bytes)
   - Não requer root

3. **test_io.c** - Valida coleta de métricas de I/O
   - ✅ Leitura de /proc/[pid]/io
   - ✅ Parsing de read_bytes, write_bytes
   - ✅ Cálculo de operações de I/O
   - Requer root (acesso a /proc/[pid]/io)

4. **test_namespace.c** - Valida análise de namespaces
   - ✅ Existência de arquivos /proc/[pid]/ns/*
   - ✅ Leitura de symlinks de namespace
   - ✅ Comparação de namespaces entre processos
   - ✅ Validação de inodes
   - Requer root (alguns namespaces)

5. **test_cgroup.c** - Valida funcionalidade de cgroups v2
   - ✅ Verificação de montagem cgroup v2
   - ✅ Controllers disponíveis (cpu, memory, io, pids)
   - ✅ Criação de cgroups
   - ✅ Leitura de limites (cpu.max, memory.max)
   - ✅ Leitura de estatísticas (cpu.stat, memory.stat)
   - ✅ Verificação de cgroup do processo
   - ✅ Controle de subtree
   - ✅ Movimentação de processos
   - Alguns testes requerem root

**Estrutura dos testes:**
```
tests/
├── test_cpu.c        # Valida leitura de /proc/[pid]/stat
├── test_memory.c     # Valida leitura de /proc/[pid]/status
├── test_io.c         # Valida leitura de /proc/[pid]/io
├── test_namespace.c  # Valida análise de namespaces
└── test_cgroup.c     # Valida funcionalidade cgroups v2
```

**Saída esperada dos testes:**

```
====================================
  Testes de Cgroups v2
====================================

=== Teste: Cgroup v2 Montado ===
  [OK] Cgroup v2 está montado em /sys/fs/cgroup
  [OK] cgroup.controllers é um arquivo regular

=== Teste: Controllers Disponíveis ===
  [OK] Conseguiu abrir cgroup.controllers
  [OK] Controller 'cpu' disponível
  [OK] Controller 'memory' disponível
  [OK] Controller 'io' disponível
  [OK] Controller 'pids' disponível

====================================
  Resumo dos Testes
====================================
  Passou: 18
  Falhou: 0

  [SUCESSO] Todos os testes passaram!
```

**Executar todos os testes de uma vez:**

```bash
# Via Makefile (executa todos automaticamente com relatório)
make run_tests

# Saída resumida ao final:
========================================
  RESUMO GERAL DOS TESTES
========================================
  Total de testes: 5
  Testes passados: 5
  Testes falhados: 0

  [SUCESSO] Todos os testes passaram!
========================================

# Testes executados (na ordem):
# 1. test_cgroup    → 18 asserções (cgroups v2)
# 2. test_cpu       → 2 asserções (métricas CPU)
# 3. test_io        → 2 asserções (métricas I/O)
# 4. test_memory    → 2 asserções (métricas memória)
# 5. test_namespace → 5 asserções (análise namespaces)
# Total: 29 asserções individuais em 5 suítes de teste
```

### Análise de Memory Leaks com Valgrind

O projeto inclui análise automatizada de memory leaks usando Valgrind.

**Instalar Valgrind:**

```bash
# Ubuntu/Debian
sudo apt install valgrind

# Arch Linux
sudo pacman -S valgrind

# Fedora
sudo dnf install valgrind
```

**Executar análise completa:**

```bash
# Análise automática de todos os componentes
make valgrind

# Ou executar script manualmente
bash scripts/valgrind_analysis.sh
```

**O script valgrind_analysis.sh testa:**
1. ✅ Monitoramento básico (5 segundos)
2. ✅ Exportação JSON
3. ✅ Exportação CSV
4. ✅ Interface TUI
5. ✅ Análise de namespaces
6. ✅ Testes unitários

**Exemplo de análise manual:**

```bash
# Análise detalhada de um processo específico
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         ./bin/monitor process 1 1 5 json

# Análise da TUI
valgrind --leak-check=full ./bin/monitor tui 1 1 5
```

**Interpretar resultados:**

```
HEAP SUMMARY:
    in use at exit: 0 bytes in 0 blocks
  total heap usage: 156 allocs, 156 frees, 45,832 bytes allocated

All heap blocks were freed -- no leaks are possible
```

- ✅ **0 bytes in use** = Sem leaks
- ✅ **allocs == frees** = Toda memória foi liberada
- ⚠️ **definitely lost** = Memory leak confirmado
- ⚠️ **possibly lost** = Possível leak (verificar)

**Relatórios gerados em:** `output/valgrind/`

---

## 📄 Licença

Projeto acadêmico desenvolvido para fins educacionais - Sistemas Operacionais, 2025.

## 📞 Suporte

Para questões sobre o projeto:
- Consulte a documentação em `docs/`
- Verifique `docs/QUICK_START.md` para início rápido
- Leia `docs/EXPERIMENTS_REPORT.md` para detalhes dos experimentos

## 🚀 Referência Rápida

### Comandos Essenciais

```bash
# Build e execução
sudo ./build.sh                              # Compilar e abrir menu

# Monitoramento
./bin/monitor tui <PID>                 # TUI em tempo real
./bin/monitor process <PID> 5 60 json   # Exportar dados

# Experimentos
sudo ./bin/monitor experiment 1         # Overhead
sudo ./bin/monitor experiment 2         # Namespaces
sudo ./bin/monitor experiment 3         # CPU throttling
sudo ./bin/monitor experiment 4         # Memory limit
sudo ./bin/monitor experiment 5         # I/O limit

# Testes
make tests                              # Compilar testes
make run_tests                          # Executar testes
make valgrind                           # Análise de leaks

# Visualização
venv/bin/python scripts/visualize.py --experiments output/graphs
```

### Estrutura de Arquivos Importantes

```
resource-monitor/
├── bin/monitor              # Binário principal
├── bin/cgroup_manager       # Gerenciador de cgroups
├── build.sh                 # Script de build
├── Makefile                 # Sistema de build
├── README.md                # Este arquivo
├── src/                     # Código fonte
│   ├── main.c              # Menu e CLI
│   ├── monitor_tui.c       # Interface TUI
│   ├── cpu_monitor.c       # Métricas CPU
│   ├── memory_monitor.c    # Métricas memória
│   └── experiment_*.c      # Experimentos
├── scripts/
│   ├── visualize.py        # Gráficos matplotlib
│   └── valgrind_analysis.sh # Análise de leaks
├── tests/                  # Testes unitários
└── output/                 # Resultados
    ├── graphs/            # Gráficos PNG
    └── experiments/       # Dados JSON/CSV
```

### Solução de Problemas Comuns

**Erro: "Permission denied" ao criar cgroup**
```bash
sudo ./bin/monitor experiment 3
```

**Erro: "matplotlib não encontrado"**
```bash
source venv/bin/activate
pip install matplotlib numpy
```

**Erro: "Process does not exist"**
```bash
# Verificar se PID existe
ps aux | grep <PID>
```

**Memory leaks detectados**
```bash
# Análise detalhada
make valgrind
cat output/valgrind/*.log
```
