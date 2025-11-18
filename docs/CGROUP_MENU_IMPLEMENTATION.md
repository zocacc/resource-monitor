# Control Group Manager - Implementação Completa

## Resumo da Implementação

A opção 3 do menu principal agora está totalmente funcional com 11 operações de gerenciamento de cgroups integradas ao `main.c`.

## Funcionalidades Implementadas

### 1. Listar Todos os Cgroups
- Lista todos os cgroups do sistema recursivamente
- Mostra hierarquia completa de `/sys/fs/cgroup`
- Exibe número de processos em cada cgroup

### 2. Listar PIDs de um Cgroup
- Mostra todos os PIDs em um cgroup específico
- Exibe tabela formatada com PID, comando e usuário
- Lê de `cgroup.procs` e extrai informações de `/proc/<pid>/`

### 3. Ler Métricas de CPU
- Exibe estatísticas de CPU do arquivo `cpu.stat`
- Métricas incluem: usage_usec, user_usec, system_usec, etc.
- Formato de tabela elegante com bordas

### 4. Ler Métricas de Memória
- Exibe estatísticas de memória do arquivo `memory.stat`
- Conversão automática de bytes para KB/MB/GB
- Métricas: anon, file, kernel, slab, etc.

### 5. Ler Métricas de I/O
- Exibe estatísticas de I/O do arquivo `io.stat`
- Métricas por dispositivo (rbytes, wbytes, rios, wios)

### 6. Criar Novo Cgroup
- Cria novo cgroup com `mkdir()` em `/sys/fs/cgroup/<nome>`
- Confirmação visual de sucesso/erro
- Permissões: 0755

### 7. Remover Cgroup
- Remove cgroup com `rmdir()`
- Requer confirmação do usuário (s/n)
- Verifica se cgroup está vazio antes de remover

### 8. Mover Processo para Cgroup
- Move processo (PID) para cgroup específico
- Escreve PID em `cgroup.procs`
- Validação de entrada (PID > 0)

### 9. Aplicar Limites de CPU
- Define porcentagem de CPU (1-100%)
- Escreve em `cpu.max` (formato: quota period)
- Period padrão: 100000 μs (100ms)
- Quota calculada: (porcentagem × period) / 100

### 10. Aplicar Limites de Memória
- Define limite de memória em MB
- Converte para bytes e escreve em `memory.max`
- Suporta valores grandes (long long)

### 11. Gerar Relatório de Utilização
- Exporta métricas para JSON
- Arquivo salvo em `output/cgroup_<nome>_report.json`
- Inclui: timestamp, CPU stats, Memory stats

## Verificação de Privilégios

- Menu verifica se o usuário é root (`geteuid() != 0`)
- Caso não seja root, exibe aviso e instruções:
  ```
  ⚠️  ATENÇÃO: Gerenciamento de cgroups requer privilégios root!
  Para acessar todas as funcionalidades, execute:
    sudo ./bin/monitor menu
  ```

## Arquivos Criados/Modificados

### Novos Arquivos
- **src/cgroup_v2.c** (283 linhas):
  - Implementações de todas as funções v2
  - Funções: `read_cpu_stat_v2()`, `read_memory_stat_v2()`, `read_io_stat_v2()`
  - Funções: `set_cpu_quota()`, `set_memory_limit()`, `add_process_to_cgroup()`
  - Função: `export_cgroup_info_to_json()`

### Arquivos Modificados
- **src/main.c**:
  - Adicionados includes: `<errno.h>`, `<sys/stat.h>`
  - Função `show_cgroup_menu()`: Menu de 11 opções
  - Função `list_all_cgroups_interactive()`: Lista cgroups do sistema
  - Função `list_cgroup_pids()`: Lista PIDs com informações de processo
  - Case 3 completo em `run_interactive_menu()`: Loop com switch de 11 operações

## Estrutura do Menu

```
╔═══════════════════════════════════════════════════════════╗
║              Control Group Manager                       ║
╠═══════════════════════════════════════════════════════════╣
║  1. Listar todos os cgroups                              ║
║  2. Listar PIDs de um cgroup                             ║
║  3. Ler métricas de CPU                                  ║
║  4. Ler métricas de memória                              ║
║  5. Ler métricas de I/O                                  ║
║  6. Criar novo cgroup                                    ║
║  7. Remover cgroup                                       ║
║  8. Mover processo para cgroup                           ║
║  9. Aplicar limites de CPU                               ║
║ 10. Aplicar limites de memória                           ║
║ 11. Gerar relatório de utilização                        ║
║                                                           ║
║  0. Voltar ao menu principal                             ║
╚═══════════════════════════════════════════════════════════╝
```

## Como Usar

```bash
# Compilar
make clean
make

# Executar (requer root para cgroups)
sudo ./bin/monitor menu

# Ou via build.sh
sudo ./build.sh
```

## Notas Técnicas

- **Cgroup v2**: Todas as operações usam cgroupv2 (`/sys/fs/cgroup`)
- **Formatação**: Tabelas com bordas Unicode para melhor visualização
- **Tratamento de Erros**: Uso consistente de `strerror(errno)`
- **Validação de Entrada**: Verificação de scanf, fgets, limites numéricos
- **Limpeza de Buffer**: `while (getchar() != '\n')` após scanf
- **Confirmações**: Operações destrutivas (remover cgroup) pedem confirmação

## Integração com Sistema Existente

- Mantém compatibilidade com `bin/cgroup_manager` standalone
- Todas as funcionalidades agora disponíveis via menu interativo
- Padrão de implementação consistente com namespace submenu (opção 2)
- Usa funções compartilhadas de `include/cgroup.h`

## Status: ✓ Implementação Completa

Todas as 11 operações foram implementadas e testadas com sucesso.
