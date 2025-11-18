# Guia de Comandos Cgroup - Resource Monitor

## 📚 Comandos Disponíveis

### 1. Criar Cgroup
```bash
./bin/monitor cgroup create <controller> <nome>
```

**Exemplo:**
```bash
# Criar cgroup no controller CPU
sudo ./bin/monitor cgroup create cpu my_cpu_group

# Criar cgroup no controller Memory
sudo ./bin/monitor cgroup create memory my_mem_group
```

---

### 2. Remover Cgroup
```bash
./bin/monitor cgroup remove <controller> <nome>
```

**Exemplo:**
```bash
sudo ./bin/monitor cgroup remove cpu my_cpu_group
```

---

### 3. Configurar Limite de CPU
```bash
./bin/monitor cgroup setcpu <nome> <period_us> <quota_us>
```

**Parâmetros:**
- `period_us`: Período em microsegundos (padrão: 100000 = 100ms)
- `quota_us`: Quota em microsegundos

**Cálculo:**
- 1 core = quota igual ao período (100000/100000 = 1.0)
- 0.5 cores = quota = 50% do período (50000/100000 = 0.5)
- 2 cores = quota = 2x o período (200000/100000 = 2.0)

**Exemplos:**
```bash
# Limitar a 0.25 cores (25% de 1 CPU)
sudo ./bin/monitor cgroup setcpu test_group 100000 25000

# Limitar a 0.5 cores
sudo ./bin/monitor cgroup setcpu test_group 100000 50000

# Limitar a 1.0 core
sudo ./bin/monitor cgroup setcpu test_group 100000 100000

# Limitar a 2.0 cores
sudo ./bin/monitor cgroup setcpu test_group 100000 200000
```

---

### 4. Configurar Limite de Memória
```bash
./bin/monitor cgroup setmem <nome> <limit_bytes>
```

**Exemplos:**
```bash
# Limitar a 100MB
sudo ./bin/monitor cgroup setmem test_group 104857600

# Limitar a 512MB
sudo ./bin/monitor cgroup setmem test_group 536870912

# Limitar a 1GB
sudo ./bin/monitor cgroup setmem test_group 1073741824
```

**Dica:** Use calculadora para converter:
- 100MB = 100 * 1024 * 1024 = 104857600 bytes
- 1GB = 1024 * 1024 * 1024 = 1073741824 bytes

---

### 5. Adicionar Processo ao Cgroup
```bash
./bin/monitor cgroup addproc <nome> <pid>
```

**Exemplo:**
```bash
# Adicionar processo atual
sudo ./bin/monitor cgroup addproc test_group $$

# Adicionar processo específico
sudo ./bin/monitor cgroup addproc test_group 12345
```

---

### 6. Exportar Informações do Cgroup para JSON
```bash
./bin/monitor cgroup export <controller> <nome> <arquivo>
```

**Exemplos:**
```bash
# Exportar informações de CPU
sudo ./bin/monitor cgroup export cpu test_group output/cpu_cgroup.json

# Exportar informações de memória
sudo ./bin/monitor cgroup export memory test_group output/mem_cgroup.json

# Exportar informações de blkio
sudo ./bin/monitor cgroup export blkio test_group output/io_cgroup.json
```

**Formato do JSON exportado:**

```json
{
  "controller": "cpu",
  "group_name": "test_group",
  "parameters": {
    "cpu.cfs_period_us": 100000,
    "cpu.cfs_quota_us": 50000,
    "cpu.shares": 1024,
    "cpu.stat": "nr_periods 1234\nnr_throttled 56\nthrottled_time 7890"
  },
  "processes": [12345, 67890]
}
```

---

## 🔬 Workflow Completo de Teste

### Exemplo: Limitando CPU de um Processo

```bash
#!/bin/bash

# 1. Criar cgroup
sudo ./bin/monitor cgroup create cpu test_cpu_limit

# 2. Configurar limite de 0.5 cores
sudo ./bin/monitor cgroup setcpu test_cpu_limit 100000 50000

# 3. Executar processo CPU-intensive em background
timeout 30s bash -c 'while true; do :; done' &
TEST_PID=$!
echo "Processo iniciado: PID $TEST_PID"

# 4. Adicionar processo ao cgroup
sudo ./bin/monitor cgroup addproc test_cpu_limit $TEST_PID

# 5. Monitorar uso de CPU
./bin/monitor monitor $TEST_PID 1 10 > output/cpu_limited.json

# 6. Exportar estado do cgroup
sudo ./bin/monitor cgroup export cpu test_cpu_limit output/cgroup_state.json

# 7. Cleanup
kill $TEST_PID 2>/dev/null
sudo ./bin/monitor cgroup remove cpu test_cpu_limit

echo "Teste concluído!"
echo "Resultados em: output/cpu_limited.json e output/cgroup_state.json"
```

---

## 📊 Análise dos Resultados

Após executar o workflow acima, você pode:

1. **Visualizar dados de monitoramento:**
   ```bash
   python3 scripts/visualize.py output/cpu_limited.json output/graphs/cpu_test
   ```

2. **Inspecionar estado do cgroup:**
   ```bash
   cat output/cgroup_state.json | jq '.'
   ```

3. **Verificar se o limite foi respeitado:**
   ```bash
   # CPU% deve estar próximo de 50% (0.5 cores)
   cat output/cpu_limited.json | jq '.[].cpu_usage_percent'
   ```

---

## ⚠️ Notas Importantes

### Permissões
Todas as operações de cgroup **requerem privilégios de root** (sudo).

### Controllers Suportados
- **cpu**: Controle de CPU (CFS scheduler)
- **memory**: Controle de memória
- **blkio**: Controle de I/O em bloco

### Localização dos Cgroups
- Caminho base: `/sys/fs/cgroup/<controller>/<nome>/`
- Parâmetros: Arquivos dentro do diretório do cgroup

### Cleanup
Sempre remova os cgroups após o uso para evitar vazamento de recursos:
```bash
sudo ./bin/monitor cgroup remove cpu my_group
```

---

## 🔍 Troubleshooting

### Erro: "Permission denied"
- **Solução:** Execute com `sudo`

### Erro: "No such file or directory"
- **Causa:** Cgroup não foi criado
- **Solução:** Execute `cgroup create` primeiro

### Erro: "Device or resource busy"
- **Causa:** Processos ainda estão no cgroup
- **Solução:** Mate os processos antes de remover o cgroup

### CPU% não respeita o limite
- **Verificar:** Sistema tem múltiplas CPUs
- **Explicação:** Limite é por CPU, não por sistema
- **Solução:** Ajuste o quota considerando o número de CPUs

---

## 📚 Referências

- [Control Groups (cgroups) - kernel.org](https://www.kernel.org/doc/Documentation/cgroup-v1/)
- [CFS Bandwidth Control](https://www.kernel.org/doc/Documentation/scheduler/sched-bwc.txt)
- [Memory Resource Controller](https://www.kernel.org/doc/Documentation/cgroup-v1/memory.txt)
