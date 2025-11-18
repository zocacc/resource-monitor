# ARCHITECTURE.md

## Resumo da Arquitetura
Este documento detalha o design do sistema de monitoramento e controle de containers Linux, conforme requisitos do projeto RA3. 

## Visão Geral
O sistema é composto por três módulos principais desenvolvidos em C, orquestrados e ampliados via scripts Python/Shell. Resultados são exportados exclusivamente em formato JSON.

## Módulos Centrais
### 1. Resource Profiler
- Monitora processos por PID
- Coleta: tempos de CPU, memória, IO, rede
- Intervalo de coleta ajustável
- Exportação: JSON
- Tratamento de erros (PID inexistente, permissões)

### 2. Namespace Analyzer
- Analisa e compara namespaces de processos
- Lista todos os namespaces ativos
- Mede overhead de criação
- Exportação: JSON

### 3. Control Group Manager
- Lê, cria e manipula cgroups
- Aplica limites: CPU, memória, IO, PIDs
- Mede precisão de throttling
- Exportação: JSON

## Scripts Python/Shell
Integram/coordenam experimentos e visualização:
- Shell: automação da execução de testes (testcpu.sh, testmemory.sh etc)
- Python: processamento dos arquivos JSON, geração de gráficos e relatórios

## Exportação de Dados
Todos os resultados/intermediários gerados por cada módulo devem seguir o padrão JSON, conforme exemplo:
```json
{
  "timestamp": "2025-10-29T21:45:00-03:00",
  "pid": 12345,
  "cpu_user": 12.3,
  "cpu_system": 1.79,
  "memory_rss": 28743,
  "io_read_bytes": 92012,
  "io_write_bytes": 1723
}
```

## Regras Técnicas
- Código C: sem warnings, bem documentado, organizado
- Não utilizar bibliotecas externas além da libc
- Exportação somente em JSON
- Makefile funcional
- README.md com instruções, exemplos e contribuições

## Estrutura de Diretórios

```
resource-monitor/
├── bin/                    # Binários compilados (gerados automaticamente)
├── obj/                    # Arquivos objeto .o (gerados automaticamente)
├── output/                 # Resultados de experimentos (versionado)
│   ├── graphs/            # Gráficos gerados (.png)
│   └── experiments/       # Dados de experimentos (.json, .csv)
├── src/                   # Código fonte C
├── include/               # Headers
├── scripts/               # Scripts Python/Shell
├── docs/                  # Documentação
└── tests/                 # Testes unitários
```

**Observações:**
- `bin/` e `obj/` são ignoradas pelo Git e criadas durante compilação
- `output/` estrutura é versionada (via `.gitkeep`), mas conteúdo gerado é ignorado
- Todos os dados experimentais são gerados localmente

## Experimentos
Scripts automáticos para:
- Medir overhead do profiler
- Validar isolamento e overhead de namespaces
- Calibrar limites de CPU/memória/io via cgroups
- Exportar métricas dos experimentos em JSON para visualização Python

## Requisitos de Segurança/Testes
- Scripts claros, validação de erros
- Tests automatizados
- Divulgação do ambiente usado (kernel/distribuição)

---
Documentação extensiva detalhando o funcionamento, scripts e integração dos componentes deve ser mantida em 'docs/' e README.
