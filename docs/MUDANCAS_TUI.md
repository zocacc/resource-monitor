# Mudanças na Interface TUI - Resource Monitor

## 📋 Resumo das Alterações

A Interface TUI (Terminal User Interface) foi aprimorada para suportar **dois modos de operação**:

1. **Modo Interativo** - Monitoramento em tempo real (comportamento anterior)
2. **Modo Temporizado** - Execução com tempo determinado e exportação automática de JSON

### Motivação

Anteriormente, havia redundância entre os comandos:
- `./bin/monitor monitor <PID> <intervalo> <duracao>` - gerava JSON mas sem interface visual
- `./bin/monitor tui <PID>` - interface visual mas sem geração de JSON

Agora a TUI **unifica ambas funcionalidades**, eliminando a redundância.

---

## 🔧 Mudanças Técnicas

### 1. Header (`include/monitor_tui.h`)
```c
// ANTES:
int run_tui(pid_t pid);

// DEPOIS:
int run_tui(pid_t pid, int interval, int duration);
```

### 2. Implementação (`src/monitor_tui.c`)

**Adicionado:**
- ✅ Parâmetros `interval` e `duration`
- ✅ Alocação dinâmica de histórico (`ResourceData *data_history`)
- ✅ Detecção de modo temporizado (`timed_mode = duration > 0`)
- ✅ Verificação de tempo decorrido no loop principal
- ✅ Exportação automática via `export_to_json()` ao finalizar
- ✅ Include de `utils.h` para função `export_to_json()`

**Lógica:**
```c
if (timed_mode) {
    // Alocar array de histórico
    num_samples = duration / refresh_interval;
    data_history = malloc(num_samples * sizeof(ResourceData));
    
    // Durante execução: armazenar cada amostra
    if (current_sample < num_samples) {
        data_history[current_sample++] = snapshot;
    }
    
    // Ao finalizar: exportar JSON
    export_to_json(data_history, current_sample, "output/monitor_output.json");
}
```

### 3. Parsing de Argumentos (`src/main.c`)

**ANTES:**
```c
if (strcmp(command, "tui") == 0) {
    if (argc != 3) {
        fprintf(stderr, "Erro: 'tui' requer PID.\n");
        return 1;
    }
    int pid = atoi(argv[2]);
    return run_tui(pid);
}
```

**DEPOIS:**
```c
if (strcmp(command, "tui") == 0) {
    if (argc < 3 || argc > 5) {
        fprintf(stderr, "Erro: 'tui' requer PID e opcionalmente intervalo e duração.\n");
        return 1;
    }
    int pid = atoi(argv[2]);
    int interval = (argc >= 4) ? atoi(argv[3]) : 0;
    int duration = (argc >= 5) ? atoi(argv[4]) : 0;
    return run_tui(pid, interval, duration);
}
```

### 4. Mensagem de Ajuda (`src/main.c`)

**Atualizado:**
```c
printf("  tui <pid> [intervalo_s] [duracao_s]    - Monitora com interface TUI (modo interativo ou tempo determinado).\n");
```

---

## 📖 Uso

### Modo Interativo (anterior - mantido)
```bash
./bin/monitor tui <PID>
```
- Interface visual em tempo real
- Atualização contínua (1 segundo)
- Encerra com 'q'
- **NÃO gera arquivo JSON**

### Modo Temporizado (novo)
```bash
./bin/monitor tui <PID> <intervalo_s> <duracao_s>
```
- Interface visual durante execução
- Encerra automaticamente após `duracao_s`
- **Exporta automaticamente** para `output/monitor_output.json`
- Formato JSON **idêntico** ao comando `monitor`

**Exemplos:**
```bash
# Monitorar por 60 segundos, amostrando a cada 1 segundo
./bin/monitor tui $$ 1 60

# Monitorar por 30 segundos, amostrando a cada 2 segundos
./bin/monitor tui 1234 2 30

# Monitoramento rápido de 10 segundos
./bin/monitor tui $$ 1 10
```

---

## ✅ Vantagens

1. **Elimina redundância** - Um comando com duas funcionalidades
2. **Experiência visual** - Mesmo em modo temporizado, usuário vê dados em tempo real
3. **Compatibilidade total** - JSON gerado é idêntico ao comando `monitor`
4. **Flexibilidade** - Usuário escolhe modo interativo ou temporizado
5. **Retrocompatibilidade** - Comando antigo `./bin/monitor tui <PID>` ainda funciona

---

## 🧪 Testes Realizados

### Teste 1: Modo Interativo
```bash
./bin/monitor tui $$
# Resultado: Interface TUI, não gera JSON ✅
```

### Teste 2: Modo Temporizado
```bash
./bin/monitor tui $$ 1 5
# Resultado: Interface TUI + JSON exportado ✅
```

### Teste 3: Validação JSON
```bash
cat output/monitor_output.json | python3 -m json.tool
# Resultado: JSON válido e bem formatado ✅
```

### Teste 4: Contagem de Amostras
```bash
./bin/monitor tui $$ 2 10
grep -c '"timestamp"' output/monitor_output.json
# Resultado: 5 amostras (10s / 2s = 5) ✅
```

---

## 📚 Documentação Atualizada

- ✅ `include/monitor_tui.h` - Header com novos parâmetros
- ✅ `src/main.c` - Mensagem de ajuda atualizada
- ✅ `docs/TUI_GUIDE.md` - Documentação completa dos dois modos
- ✅ `README.md` - Exemplos de uso atualizados
- ✅ `scripts/demo_presentation.sh` - Referências atualizadas

---

## 🎯 Conclusão

A TUI agora é uma **ferramenta completa** que combina:
- Visualização interativa em tempo real
- Exportação estruturada de dados (JSON)
- Flexibilidade de uso (interativo ou temporizado)

Isso torna desnecessário o uso do comando `./bin/monitor monitor` na maioria dos casos, pois a TUI oferece a mesma funcionalidade com uma experiência visual superior.
