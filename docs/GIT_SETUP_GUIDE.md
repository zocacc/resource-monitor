# Configuração de Commits por Aluno

Este guia explica como configurar o repositório Git com commits organizados por aluno.

## 📋 Pré-requisitos

- Git instalado
- Repositório GitHub criado (ou use o script para criar)
- Nomes e emails dos 4 alunos

## 🚀 Passo a Passo

### 1. Editar Informações dos Alunos

Abra o arquivo `scripts/setup_git_commits.sh` e edite as linhas 31-42:

```bash
# PERSONALIZE ESTAS INFORMAÇÕES:
ALUNO1_NAME="João Silva"
ALUNO1_EMAIL="joao.silva@email.com"

ALUNO2_NAME="Maria Santos"
ALUNO2_EMAIL="maria.santos@email.com"

ALUNO3_NAME="Pedro Oliveira"
ALUNO3_EMAIL="pedro.oliveira@email.com"

ALUNO4_NAME="Ana Costa"
ALUNO4_EMAIL="ana.costa@email.com"
```

### 2. Executar o Script

```bash
cd /home/zocac/projects/sistemas-operacionais/projeto/resource-monitor
./scripts/setup_git_commits.sh
```

Este script irá:
- ✅ Inicializar o repositório Git
- ✅ Criar 16 commits organizados por aluno (4 por aluno)
- ✅ Adicionar 3 commits colaborativos finais
- ✅ Organizar o histórico conforme a divisão de tarefas

### 3. Criar Repositório no GitHub

**Opção A: Via GitHub CLI**
```bash
gh repo create resource-monitor --public --description "Sistema de monitoramento de recursos Linux com namespaces e cgroups"
```

**Opção B: Via Web Interface**
1. Acesse https://github.com/new
2. Nome: `resource-monitor`
3. Descrição: "Sistema de monitoramento de recursos Linux com namespaces e cgroups"
4. Público
5. Não inicialize com README (já temos)
6. Crie o repositório

### 4. Conectar e Fazer Push

```bash
# Adicionar remote (substitua SEU_USUARIO)
git remote add origin https://github.com/SEU_USUARIO/resource-monitor.git

# Push para GitHub
git push -u origin main
```

## 📊 Estrutura de Commits

### Aluno 1: Resource Profiler + Integração (4 commits)

1. **feat: adiciona estrutura inicial do projeto e Makefile**
   - Makefile, build.sh, headers, utils

2. **feat: implementa coleta de métricas de CPU**
   - cpu_monitor.c, cálculos de percentual

3. **feat: implementa coleta de métricas de memória**
   - memory_monitor.c, VSZ, RSS

4. **feat: integra componentes de monitoramento**
   - main.c, process_monitor.c, menu interativo

### Aluno 2: Resource Profiler + Testes (4 commits)

1. **feat: implementa coleta de métricas de I/O**
   - io_monitor.c, taxas de leitura/escrita

2. **feat: implementa coleta de métricas de rede**
   - network_monitor.c, TX/RX

3. **test: adiciona programas de teste de recursos**
   - test_cpu.c, test_memory.c, test_io.c

4. **test: adiciona scripts de validação e comparação**
   - comparetools.sh, valgrind_analysis.sh

### Aluno 3: Namespace Analyzer + Experimentos (4 commits)

1. **feat: implementa análise de namespaces**
   - namespace_analyzer.c, listagem e comparação

2. **feat: adiciona experimento 2 - isolamento via namespaces**
   - Validação de todos os tipos de namespaces

3. **feat: adiciona experimento 1 - overhead de monitoramento**
   - experiment_overhead.c, benchmark

4. **docs: adiciona documentação de experimentos de namespaces**
   - EXPERIMENTS_REPORT.md, metodologia

### Aluno 4: Control Group Manager + Análise (4 commits)

1. **feat: implementa coleta de métricas de cgroups v2**
   - cgroup_v2.c, cpu.stat, memory.stat, io.stat

2. **feat: implementa criação e configuração de cgroups**
   - cgroup_manager.c, menu interativo

3. **feat: adiciona experimentos 3, 4 e 5 - throttling de recursos**
   - CPU, memória e I/O throttling

4. **feat: adiciona geração de visualizações e relatórios**
   - visualize.py, gráficos matplotlib

### Commits Colaborativos (3 commits)

1. **feat: adiciona interface TUI em tempo real**
2. **docs: adiciona documentação completa do projeto**
3. **chore: refinamentos finais e ajustes**

## 🔍 Verificar Commits

```bash
# Ver todos os commits
git log --oneline --graph --all

# Commits por autor
git shortlog -s -n --all

# Detalhes de um commit específico
git show <commit-hash>
```

## 📝 Exemplo de Saída

```
Aluno 1 - Integrador: 4 commits
Aluno 2 - Testes: 4 commits
Aluno 3 - Namespaces: 4 commits
Aluno 4 - Cgroups: 4 commits
Colaborativos: 3 commits
Total: 19 commits
```

## 🎯 Divisão Conforme Especificação

### Aluno 1: Resource Profiler + Integração ✅
- ✅ Implementar coleta de CPU e Memory
- ✅ Implementar cálculos de percentuais e taxas
- ✅ Integrar os três componentes
- ✅ Criar Makefile geral

### Aluno 2: Resource Profiler + Testes ✅
- ✅ Implementar coleta de I/O e Network
- ✅ Criar programas de teste (CPU, Memory, I/O intensive)
- ✅ Validar precisão das medições
- ✅ Documentar metodologia de testes

### Aluno 3: Namespace Analyzer + Experimentos ✅
- ✅ Implementar análise de namespaces
- ✅ Criar experimentos com diferentes tipos de namespaces
- ✅ Medir overhead de isolamento
- ✅ Documentar resultados experimentais

### Aluno 4: Control Group Manager + Análise ✅
- ✅ Implementar coleta de métricas de cgroups
- ✅ Implementar criação e configuração de cgroups
- ✅ Conduzir experimentos de throttling
- ✅ Gerar relatórios e visualizações

## ⚠️ Notas Importantes

1. **Emails**: Use emails institucionais ou do GitHub dos alunos
2. **Nomes**: Use nomes completos reais
3. **Ordem**: Os commits serão criados na ordem cronológica correta
4. **Histórico**: O script preserva a lógica de desenvolvimento incremental

## 🆘 Troubleshooting

### "fatal: not a git repository"
O script cuida disso automaticamente, inicializando o repositório.

### "Author identity unknown"
Configure globalmente:
```bash
git config --global user.name "Seu Nome"
git config --global user.email "seu@email.com"
```

### Refazer commits
```bash
rm -rf .git
./scripts/setup_git_commits.sh
```

## 📚 Recursos Adicionais

- [Git Commit Best Practices](https://www.conventionalcommits.org/)
- [Como Escrever Boas Mensagens de Commit](https://chris.beams.io/posts/git-commit/)
- [GitHub Flow](https://guides.github.com/introduction/flow/)
