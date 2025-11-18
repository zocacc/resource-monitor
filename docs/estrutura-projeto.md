# Estrutura Básica do Projeto: Monitoramento e Análise de Containers

## Introdução
Este projeto implementa um sistema de monitoramento, análise e controle de recursos para containers Linux, utilizando as linguagens C, Python e Shell Script. O foco é atender todos os requisitos definidos no arquivo 'Conteiners-Recursos-RA3.pdf', com exportação dos resultados em formato JSON.

## Estrutura de Diretórios
```
resource-monitor/
├─ README.md
├─ Makefile
├─ docs/
│  └─ ARCHITECTURE.md
├─ include/
│  ├─ monitor.h
│  ├─ namespace.h
│  └─ cgroup.h
├─ src/
│  ├─ cpumonitor.c
│  ├─ memorymonitor.c
│  ├─ iomonitor.c
│  ├─ namespaceanalyzer.c
│  ├─ cgroupmanager.c
│  └─ main.c
├─ tests/
│  ├─ testcpu.c
│  ├─ testmemory.c
│  └─ testio.c
├─ scripts/
│  ├─ visualize.py
│  └─ comparetools.sh
```

## Componentes Principais
### Resource Profiler (C)
    - Coleta métricas detalhadas (CPU, memória, IO, rede) de processos por PID
    - Intervalo de coleta configurável
    - Exporta métricas em JSON
    - Tratamento robusto de erros
    - Integração com scripts para experimentos

### Namespace Analyzer (C)
    - Mapeia e compara namespaces de processos
    - Mede overhead de criação de namespaces
    - Exporta estrutura e isolamento em JSON

### Control Group Manager (C)
    - Lê, cria e configura cgroups experimentais
    - Aplica limites (CPU/Memory/IO)
    - Exporta métricas e limites aplicados em JSON

### Scripts Auxiliares (Python/Shell)
    - Orquestram experimentos, automação de testes e visualização de resultados
    - Python: visualização dos dados JSON e geração de relatórios
    - Shell: integração do processo de coleta, execução de workloads e automação

## Fluxo Operacional
1. Scripts (Python/Shell) disparam experimentos e coleta
2. Resource Profiler monitora processos e exporta dados em JSON
3. Namespace Analyzer analisa isolamento e exporta relatórios em JSON
4. Control Group Manager aplica/configura limites e exporta métricas em JSON
5. Dados JSON são utilizados para geração de relatórios e visualizações em Python

## Exportação de Dados
Todas as métricas, mapas de namespaces e testes com cgroups devem ser salvos em formato JSON em arquivos na pasta 'output/' ou similar, para facilitar integração e análise posterior.

## Requisitos Técnicos
- Código em C deve compilar sem warnings (-Wall -Wextra)
- Não usar bibliotecas externas (apenas libc padrão)
- Scripts compatíveis com shell padrão (bash)
- README detalhado, com instruções de compilação, uso, exemplos e contribuições de autores

## Experimentos Obrigatórios
1. Overhead do monitoramento (comparar execução de workload com e sem profiler)
2. Efetividade do isolamento de namespaces e tempo de criação
3. Testes de limitação de CPU (diversas cotas)
4. Teste de OOM Killer (limite de memória)
5. Teste restrição de IO

## Documentação e Qualidade
- Código documentado, modular e de fácil manutenção
- Makefile para automação de compilação
- Testes automatizados com scripts mencionados na pasta 'tests/'
- Validação com valgrind (bônus)
- Visualização de métricas dos experimentos em Python

## Critérios de Avaliação
- Funcionalidade e correção dos módulos
- Eficiência e robustez
- Documentação completa
- Cumprimento dos experimentos
- Exportação dos dados em JSON

---

Esta estrutura pode ser ajustada conforme o progresso do desenvolvimento. Para detalhamento de implementação (funções, exemplos de código), solicite o componente específico.
