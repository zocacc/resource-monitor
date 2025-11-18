# Menu de Control Groups Atualizado

## ✓ Modificações Realizadas

### Opções Unificadas:

**ANTES (11 opções):**
```
 1) Listar todos os cgroups do sistema
 2) Listar PIDs de um cgroup
 3) Ler métricas de CPU de um cgroup         ┐
 4) Ler métricas de Memória de um cgroup     ├─ UNIFICADAS
 5) Ler métricas de I/O de um cgroup         ┘
 6) Criar novo cgroup experimental
 7) Remover cgroup experimental
 8) Mover processo para cgroup
 9) Aplicar limites de CPU                    ┐
10) Aplicar limites de Memória               ├─ UNIFICADAS
11) Gerar relatório de utilização            ┘
 0) Voltar ao menu principal
```

**DEPOIS (8 opções):**
```
╔═══════════════════════════════════════════════════════════╗
║              Control Group Manager (v2)                  ║
╠═══════════════════════════════════════════════════════════╣
║  1) Listar todos os cgroups do sistema                   ║
║  2) Listar PIDs de um cgroup                             ║
║  3) Ler métricas de um cgroup (CPU/Memória/I/O)          ║ ← Unificação 1
║  4) Criar novo cgroup experimental                       ║
║  5) Remover cgroup experimental                          ║
║  6) Mover processo para cgroup                           ║
║  7) Aplicar limites de recursos (CPU/Memória)            ║ ← Unificação 2
║  8) Gerar relatório de utilização                        ║
║  0) Voltar ao menu principal                             ║
╚═══════════════════════════════════════════════════════════╝
```

## 📋 Detalhes das Opções Unificadas

### Opção 3: Ler Métricas (CPU/Memória/I/O)
- **Entrada:** Nome do cgroup
- **Saída:** 
  - CPU Statistics (cpu.stat)
  - Memory Statistics (memory.stat com conversão KB/MB/GB)
  - I/O Statistics (io.stat por dispositivo)
- **Implementação:** Chama 3 funções sequencialmente:
  - `read_cpu_stat_v2()`
  - `read_memory_stat_v2()`
  - `read_io_stat_v2()`

### Opção 7: Aplicar Limites (CPU/Memória)
- **Submenu interativo:**
  ```
  1) Limite de CPU
  2) Limite de Memória
  3) Ambos (CPU + Memória)
  ```
- **CPU:**
  - Entrada: Porcentagem (1-100%)
  - Conversão: % → quota/period (μs)
  - Arquivo: `cpu.max`
- **Memória:**
  - Entrada: MB
  - Conversão: MB → bytes
  - Arquivo: `memory.max`

## 🎯 Benefícios

1. **Redução de 11 → 8 opções** (27% menos opções)
2. **Menu mais compacto e intuitivo**
3. **Operações relacionadas agrupadas**
4. **Workflow simplificado:**
   - Ver todas as métricas de uma vez (opção 3)
   - Aplicar múltiplos limites juntos (opção 7)

## 🔧 Arquivos Modificados

- `src/main.c`: Menu e cases unificados
- `src/cgroup_v2.c`: Funções de leitura já existentes

## ✅ Status

- ✓ Compilação bem-sucedida
- ✓ Menu formatado com bordas
- ✓ Lógica unificada implementada
- ✓ Validação de entrada mantida
- ✓ Mensagens de erro/sucesso preservadas
