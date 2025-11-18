# Valgrind Analysis Guide - Resource Monitor

## 📋 Índice
- [Visão Geral](#visão-geral)
- [Instalação](#instalação)
- [Uso Básico](#uso-básico)
- [Tipos de Testes](#tipos-de-testes)
- [Interpretando Resultados](#interpretando-resultados)
- [Casos de Uso](#casos-de-uso)
- [Troubleshooting](#troubleshooting)

---

## 🎯 Visão Geral

O Valgrind é uma ferramenta de instrumentação para detecção de:
- **Memory leaks** (vazamentos de memória)
- **Invalid memory access** (uso de memória após liberação)
- **Uninitialized memory reads** (leitura de memória não inicializada)
- **Double frees** (liberação dupla de memória)
- **Buffer overflows** (estouro de buffer)

Este projeto implementa análise automatizada com Valgrind para garantir qualidade do código C.

---

## 📦 Instalação

### Ubuntu/Debian
```bash
sudo apt update
sudo apt install valgrind
```

### Arch Linux
```bash
sudo pacman -S valgrind
```

### Fedora/RHEL
```bash
sudo dnf install valgrind
```

### Verificar Instalação
```bash
valgrind --version
# Saída esperada: valgrind-3.X.X
```

---

## 🚀 Uso Básico

### Método 1: Via Makefile (Recomendado)
```bash
cd resource-monitor
make valgrind
```

### Método 2: Via Script Direto
```bash
bash scripts/valgrind_analysis.sh
```

### Método 3: Comando Manual
```bash
valgrind --leak-check=full --show-leak-kinds=all ./bin/monitor
```

---

## 🧪 Tipos de Testes

### 1. Teste de Help (Teste Básico)
**O que faz:** Executa o monitor sem argumentos para exibir help  
**Verifica:** Memory leaks na função `print_usage()`

```bash
# No menu interativo, escolha opção 1
```

**Comando equivalente:**
```bash
valgrind --leak-check=full ./bin/monitor
```

---

### 2. Teste com PID Inválido
**O que faz:** Tenta monitorar um processo inexistente (PID 999999)  
**Verifica:** Tratamento de erros sem leaks

```bash
# No menu interativo, escolha opção 2
```

**Comando equivalente:**
```bash
valgrind --leak-check=full ./bin/monitor monitor 999999 1 2
```

**Comportamento esperado:**
- Programa detecta PID inválido
- Retorna erro apropriado
- Libera toda memória alocada
- Valgrind reporta: "All heap blocks were freed -- no leaks are possible"

---

### 3. Teste de Namespace
**O que faz:** Lista namespaces do processo init (PID 1)  
**Verifica:** Leitura de `/proc/[pid]/ns/*` sem leaks

```bash
# No menu interativo, escolha opção 3
```

**Comando equivalente:**
```bash
valgrind --leak-check=full ./bin/monitor namespace list 1
```

---

### 4. Testes Unitários
**O que faz:** Executa test_cpu, test_memory, test_io com Valgrind  
**Verifica:** Unit tests não causam leaks

```bash
# No menu interativo, escolha opção 4
```

**Pré-requisito:**
```bash
make tests  # Compilar testes primeiro
```

**Comando equivalente:**
```bash
valgrind --leak-check=full ./bin/tests/test_cpu
valgrind --leak-check=full ./bin/tests/test_memory
valgrind --leak-check=full ./bin/tests/test_io
```

---

### 5. Teste Completo (Processo Real)
**O que faz:** Monitora processo real (sleep 10) por 5 segundos  
**Verifica:** Ciclo completo de monitoramento sem leaks

```bash
# No menu interativo, escolha opção 5
```

**Fluxo:**
1. Script inicia `sleep 10` em background
2. Valgrind executa monitor no PID do sleep
3. Monitor coleta métricas por 5 segundos
4. Exporta dados para JSON
5. Script mata o processo sleep
6. Valgrind verifica leaks

---

### 6. Executar Todos os Testes
**O que faz:** Executa testes 1-5 sequencialmente  
**Gera:** Relatório consolidado

```bash
# No menu interativo, escolha opção 'a'
```

**Duração estimada:** ~2-3 minutos

---

## 📊 Interpretando Resultados

### Formato do Log

Cada teste gera 2 arquivos:
- `output/valgrind/<test_name>.log` - Log texto detalhado
- `output/valgrind/<test_name>.xml` - XML para ferramentas

### Lendo o Summary

```bash
# Gerar resumo
bash scripts/valgrind_analysis.sh
# Escolha opção 'r'
```

Exemplo de `output/valgrind/SUMMARY.md`:

```markdown
### test_help

| Métrica | Valor |
|---------|-------|
| Erros | 0 |
| Definitely Lost | 0 bytes |
| Indirectly Lost | 0 bytes |
| Possibly Lost | 0 bytes |
| Still Reachable | 0 bytes |

**Status:** ✅ PASSOU (sem leaks ou erros)
```

---

### Tipos de Memory Leaks

#### 1. Definitely Lost ❌
**Mais grave.** Memória alocada e nunca liberada, sem ponteiros para ela.

**Exemplo:**
```c
char *buffer = malloc(100);
// Esqueceu de dar free(buffer)
```

**Ação:** CORRIGIR IMEDIATAMENTE - adicionar `free()`.

---

#### 2. Indirectly Lost ⚠️
Memória alocada indiretamente (ex: dentro de struct) e perdida.

**Exemplo:**
```c
typedef struct {
    char *data;
} MyStruct;

MyStruct *s = malloc(sizeof(MyStruct));
s->data = malloc(100);
free(s);  // ❌ Perdeu s->data!
```

**Ação:** Liberar sub-alocações antes da estrutura principal.

---

#### 3. Possibly Lost ⚠️
Ponteiros para meio do bloco alocado, não para o início.

**Exemplo:**
```c
char *buffer = malloc(100);
buffer += 10;  // ❌ Perdeu ponteiro original!
```

**Ação:** Manter ponteiro original para dar free().

---

#### 4. Still Reachable ℹ️
Memória ainda acessível no final do programa.

**Geralmente OK** - memória global/estática que sobrevive até o exit().

**Quando corrigir:** Se for muito grande (>1MB).

---

### Exemplo de Leak Detectado

```
==12345== 100 bytes in 1 blocks are definitely lost in loss record 1 of 1
==12345==    at 0x4C2FB0F: malloc (in /usr/lib/valgrind/vgpreload_memcheck-amd64-linux.so)
==12345==    by 0x108A2E: get_cpu_stats (cpu_monitor.c:42)
==12345==    by 0x108D5C: main (main.c:123)
```

**Interpretação:**
- **100 bytes** vazaram
- Alocados em **cpu_monitor.c linha 42** (função `get_cpu_stats`)
- Chamado de **main.c linha 123**

**Ação:**
```c
// cpu_monitor.c:42
char *buffer = malloc(100);
// ... usar buffer ...
free(buffer);  // ← ADICIONAR ESTA LINHA
```

---

### Exemplo de Invalid Read

```
==12345== Invalid read of size 4
==12345==    at 0x108A5F: parse_cpu_stat (cpu_monitor.c:67)
==12345==  Address 0x52345a0 is 0 bytes after a block of size 100 alloc'd
```

**Interpretação:**
- Tentou ler 4 bytes **após o fim** do buffer
- Buffer tem 100 bytes, tentou acessar posição 100+

**Ação:**
```c
// Verificar limites antes de acessar
if (index < buffer_size) {
    value = buffer[index];
}
```

---

## 🔍 Casos de Uso

### Caso 1: Novo Código Implementado
**Situação:** Acabou de implementar `export_cgroup_info_to_json()`  
**Objetivo:** Garantir que não há leaks

```bash
# 1. Compilar
make clean && make all

# 2. Rodar Valgrind
make valgrind

# 3. Escolher teste relevante
# Opção 3 para testar cgroup

# 4. Verificar output/valgrind/test_*.log
grep "definitely lost" output/valgrind/*.log
```

**Resultado esperado:**
```
definitely lost: 0 bytes in 0 blocks
```

---

### Caso 2: Debugging de Segfault
**Situação:** Programa crasha com segmentation fault  
**Objetivo:** Encontrar a linha exata do erro

```bash
# Compilar com debug symbols
make clean
CFLAGS="-Wall -Wextra -Iinclude -g" make all

# Rodar com Valgrind
valgrind --track-origins=yes ./bin/monitor <args>
```

**Valgrind mostrará:**
- Linha exata do crash
- Stack trace completo
- Origem da memória inválida

---

### Caso 3: Otimização de Memória
**Situação:** Programa usa muita memória  
**Objetivo:** Identificar alocações grandes

```bash
valgrind --tool=massif ./bin/monitor monitor 1234 1 10
ms_print massif.out.<pid>
```

**Gera:** Gráfico de uso de memória ao longo do tempo.

---

## ⚠️ Troubleshooting

### Erro: "valgrind: command not found"
**Causa:** Valgrind não instalado  
**Solução:** Ver seção [Instalação](#instalação)

---

### Erro: "Permission denied" ao acessar /proc
**Causa:** Alguns arquivos em /proc requerem root  
**Solução:** 
```bash
sudo bash scripts/valgrind_analysis.sh
```

---

### Muitos "Still Reachable" leaks
**Causa:** Memória global de bibliotecas (normal)  
**Solução:** Ignorar se < 1KB. Focar em "Definitely Lost".

**Suprimir leaks conhecidos:**
```bash
valgrind --suppressions=valgrind.supp ./bin/monitor
```

Criar `valgrind.supp`:
```
{
   ignore_libc_leaks
   Memcheck:Leak
   ...
   obj:/lib/x86_64-linux-gnu/libc-2.31.so
}
```

---

### Teste muito lento
**Causa:** Valgrind instrumenta cada instrução (10-30x mais lento)  
**Solução:** Normal. Rodar apenas quando necessário.

**Para testes rápidos:**
```bash
# Sem Valgrind
./bin/monitor monitor 1234 1 5

# Com Valgrind (mais lento)
valgrind ./bin/monitor monitor 1234 1 5
```

---

### "Conditional jump depends on uninitialized value"
**Causa:** Variável usada antes de ser inicializada

```c
int value;
if (value > 10) {  // ❌ value não foi inicializado!
    // ...
}
```

**Solução:**
```c
int value = 0;  // ✅ Inicializar sempre
if (value > 10) {
    // ...
}
```

---

## 📚 Referências

- [Valgrind Quick Start](https://valgrind.org/docs/manual/quick-start.html)
- [Memcheck Manual](https://valgrind.org/docs/manual/mc-manual.html)
- [Common Errors](https://valgrind.org/docs/manual/mc-manual.html#mc-manual.errormsgs)

---

## ✅ Checklist de Qualidade

Após rodar Valgrind, verificar:

- [ ] **0 definitely lost** - Sem leaks críticos
- [ ] **0 invalid reads/writes** - Sem acesso inválido
- [ ] **0 uninitialized values** - Todas variáveis inicializadas
- [ ] **< 1KB still reachable** - Leaks globais mínimos
- [ ] **ERROR SUMMARY: 0 errors** - Zero erros totais

Se todos ✅, código está **memory-safe**! 🎉

---

## 🏆 Pontuação Extra

**Critério do RA3:** Análise com Valgrind = **+5 pontos**

**Para obter os pontos:**
1. ✅ Instalar Valgrind
2. ✅ Executar `make valgrind`
3. ✅ Escolher opção 'a' (todos os testes)
4. ✅ Gerar relatório com opção 'r'
5. ✅ Incluir `output/valgrind/SUMMARY.md` na entrega

**Exemplo de evidência:**
```bash
# Terminal output
$ make valgrind
Starting Valgrind analysis...
✓ Valgrind encontrado: valgrind-3.19.0
✓ Binário encontrado: ./bin/monitor

▶ Teste: test_help
  ✓ Sem leaks ou erros

▶ Teste: test_invalid_pid
  ✓ Sem leaks ou erros

...

✓ Resumo gerado: output/valgrind/SUMMARY.md
```

**Screenshot recomendado:** Capturar output do terminal mostrando todos os testes passando.
