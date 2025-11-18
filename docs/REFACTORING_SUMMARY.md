# Refatoração: Separação de Lógica e Apresentação

## Resumo da Refatoração

O projeto foi refatorado para separar a **lógica de negócio** (em C) da **lógica de apresentação** (em Shell Script), seguindo boas práticas de engenharia de software.

---

## Problemas Identificados

### Antes da Refatoração:
- ✗ `demo_presentation.sh` com **>2000 linhas**
- ✗ Lógica de coleta de dados misturada com apresentação
- ✗ Experimentos implementados em Bash (difícil manutenção)
- ✗ Processamento de dados em shell script (ineficiente)
- ✗ Duplicação de código entre experimentos

---

## Solução Implementada

### Arquivos Criados:

#### 1. **include/experiments.h**
Interface para os 5 experimentos:
```c
void run_experiment_monitoring_overhead(const char *output_file);
void run_experiment_namespace_isolation(const char *output_file);
void run_experiment_cpu_throttling(const char *output_file);
void run_experiment_memory_limit(const char *output_file);
void run_experiment_io_limit(const char *output_file);
```

#### 2. **src/experiments.c**
Implementação completa dos experimentos em C:
- **Experimento 1** (Overhead de Monitoramento): ✅ **Totalmente implementado**
  - Workload CPU intensivo (1M iterações)
  - Medição com `clock_gettime()` (precisão nanosegundo)
  - Monitoramento com fork/waitpid
  - Sampling em 2 intervalos (1s e 100ms)
  - Cálculo de overhead e latência
  - Geração de JSON estruturado

- **Experimentos 2-5**: Mensagens informativas
  - Ainda utilizam script shell (dependências externas: unshare, ip, ipcs, etc.)
  - Podem ser migrados gradualmente para C conforme necessidade

#### 3. **src/main.c** (Atualizado)
Novo comando adicionado:
```c
./bin/monitor experiment <1-5>
```

Executa experimentos via funções C ao invés de scripts complexos.

#### 4. **scripts/demo_presentation_v2.sh**
Script simplificado (**~600 linhas** vs. 2000+ original):
- Interface de menu limpa
- Delega processamento para binários C
- Apenas exibição e navegação
- Mantém compatibilidade com funcionalidades anteriores

---

## Comparação: Antes vs. Depois

### Experimento 1: Overhead de Monitoramento

#### ANTES (Shell Script - ~250 linhas):
```bash
# Criar workload
cat > /tmp/benchmark.sh <<EOF
for i in {1..1000000}; do result=$((i*i*i)); done
EOF

# Executar e medir
START_TIME=$(date +%s%N)
/tmp/benchmark.sh &
WL_PID=$!
wait $WL_PID
END_TIME=$(date +%s%N)

# Monitoramento com loop while
while kill -0 $WL_PID; do
    SAMPLE_START=$(date +%s%N)
    ps -p $WL_PID -o %cpu,%mem > /dev/null
    cat /proc/$WL_PID/stat > /dev/null
    SAMPLE_END=$(date +%s%N)
    sleep 1
done

# Calcular overhead com awk
OVERHEAD=$(awk "BEGIN {printf \"%.2f\", ...}")

# Gerar JSON manualmente
cat > output.json <<JSONEOF
{
  "overhead": "$OVERHEAD",
  ...
}
JSONEOF
```

#### DEPOIS (C - ~200 linhas, muito mais eficiente):
```c
void run_experiment_monitoring_overhead(const char *output_file) {
    // Workload embutido
    cpu_workload(1000000);
    
    // Medição precisa
    long long start = get_timestamp_ns();
    cpu_workload(1000000);
    long long end = get_timestamp_ns();
    
    // Fork para monitoramento real
    pid_t pid = fork();
    if (pid == 0) {
        cpu_workload(1000000);
    } else {
        // Sampling com precisão
        while (waitpid(pid, NULL, WNOHANG) == 0) {
            read_proc_stats(pid);
            usleep(interval);
        }
    }
    
    // JSON com fprintf estruturado
    fprintf(json, "{\n");
    fprintf(json, "  \"overhead_percentage\": %.2f\n", overhead_pct);
    fprintf(json, "}\n");
}
```

---

## Benefícios da Refatoração

### ✅ **Desempenho**
- Medições com precisão de **nanosegundos** (vs. milissegundos no shell)
- Sem overhead de spawning de processos shell extras
- Processamento direto em memória

### ✅ **Manutenibilidade**
- Código organizado por funcionalidade
- Fácil de testar unitariamente
- Lógica reutilizável

### ✅ **Extensibilidade**
- Novos experimentos: adicionar função em `experiments.c`
- Novo comando no main.c
- Script shell não precisa mudar

### ✅ **Clareza**
- Separação clara: C = processamento, Shell = apresentação
- Script shell reduzido de 2000+ para ~600 linhas
- Menos duplicação de código

---

## Uso

### Executar experimento diretamente:
```bash
./bin/monitor experiment 1
```

### Executar via interface:
```bash
./scripts/demo_presentation_v2.sh
# Escolher opção 4 (Experimentos)
# Escolher experimento desejado
```

### Ver resultados:
```bash
cat output/experiments/exp1_monitoring_overhead.json | jq .
```

---

## Estrutura de Arquivos

```
resource-monitor/
├── include/
│   └── experiments.h          # ← NOVO: Interface dos experimentos
├── src/
│   ├── experiments.c          # ← NOVO: Implementação em C
│   ├── main.c                 # ← ATUALIZADO: Comando 'experiment'
│   └── ...
├── scripts/
│   ├── demo_presentation.sh           # Original (backup)
│   ├── demo_presentation.sh.backup    # Backup automático
│   └── demo_presentation_v2.sh        # ← NOVO: Versão refatorada
└── output/
    └── experiments/
        └── exp1_monitoring_overhead.json
```

---

## Próximos Passos (Opcional)

### Migração dos Experimentos 2-5:
Podem ser migrados gradualmente para C usando APIs Linux:
- **Experimento 2**: `unshare()`, `setns()` (sys/syscalls)
- **Experimento 3-5**: Já utilizam cgroup_manager (C), podem ser integrados

### Testes Automatizados:
```c
// tests/test_experiments.c
void test_experiment1_generates_json() {
    run_experiment_monitoring_overhead("test_output.json");
    assert(file_exists("test_output.json"));
    assert(json_is_valid("test_output.json"));
}
```

---

## Conclusão

✅ **Refatoração bem-sucedida!**
- Lógica movida para C (performance e manutenibilidade)
- Shell script simplificado (apenas apresentação)
- Experimento 1 completamente funcional
- Arquitetura escalável para novos experimentos

**Redução**: 2000+ linhas → 600 linhas (script) + 400 linhas (C bem estruturado)
