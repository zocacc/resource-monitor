RELATÓRIO DE AVALIAÇÃO - ATIVIDADE RA2
==========================================

GRUPO: grupo15_RA2

ALUNOS:
- zocacc
- Enzo Capellari

LINGUAGEM: Python

NOTA BASE: 10

Justificativa: O projeto implementa TODAS as funcionalidades solicitadas com alta qualidade técnica. A estrutura de diretórios segue perfeitamente a especificação (/core/, /algorithms/, /simulation/, /texts/, /docs/, ra2_main.py, README.md). Foram encontrados exatamente 100 textos, todos com mais de 1000 palavras (verificados: texto 25 com 3159 palavras, texto 50 com 3077 palavras). Quatro algoritmos de cache foram implementados: FIFO, LRU, LFU e MRU, sendo MRU um algoritmo extra além dos básicos solicitados. A interface de usuário funciona com entrada simples de linha de comando (número do texto, 0 para sair, -1 para simulação). O modo de simulação está completamente implementado com 3 usuários, 200 requisições por usuário, três padrões de acesso (uniforme, Poisson e ponderado com 43% para textos 30-40), coleta de métricas (hits, misses, tempos) e geração automática de relatórios e gráficos. O fluxo Git foi exemplar com 5 Pull Requests. O trabalho foi bem balanceado entre os membros (zocacc: 6 commits, Enzo Capellari: 5 commits). A nota base é 9 pois foram utilizados apenas algoritmos conhecidos (FIFO, LRU, LFU) mais MRU, e a interface tem uma linha de comando simples conforme especificado.

Problemas e DEMÉRITOS APLICADOS:

PROBLEMAS IDENTIFICADOS:

1. Identificação de alunos incompleta no código-fonte (ra2_main.py:6)
   - O arquivo principal contém placeholder não preenchido: "Autores: [Nomes dos integrantes do grupo]"
   - Esta linha deveria identificar claramente os nomes dos desenvolvedores
   - README.md também não lista explicitamente os nomes dos alunos, apenas menciona "Feito com para a disciplina de Sistemas Operacionais" (linha 116)
   - Os nomes foram identificados apenas através do histórico Git: zocacc e Enzo Capellari

2. Interface contém modo de configuração adicional (ra2_main.py:88-91, 147-172)
   - Além das entradas básicas (número do texto, 0, -1), existe entrada -2 para "Modo de Configuração"
   - Este modo apresenta um menu com subopções (alterar algoritmo, configurar atraso de disco)
   - Embora seja uma funcionalidade adicional útil, o requisito especificava "interface de usuário com apenas uma linha de comando, ou seja, o usuário não precisa entrar em menus ou submenus para fazer as operações básicas"
   - Contudo, as operações BÁSICAS (solicitar texto, sair, simulação) funcionam perfeitamente sem menus
   - O menu de configuração é OPCIONAL e não interfere no funcionamento básico
   - Portanto, este não é considerado um problema crítico, mas uma extensão da interface

DEMÉRITOS APLICADOS:

1. Documentação Individual Incompleta no README.md: -1.0 ponto
   - ra2_main.py linha 6 contém placeholder não preenchido: "Autores: [Nomes dos integrantes do grupo]"
   - README.md não lista explicitamente os nomes dos alunos que desenvolveram o projeto
   - A identificação dos desenvolvedores só foi possível através da análise do histórico Git
   - Conforme especificação: "Verifique se o README.md está atualizado e contém as informações necessárias para a documentação e uso do projeto"

Nenhum outro demérito foi aplicado:

- Estrutura de Diretórios: Perfeita, todas as pastas corretas
- Histórico de Commits Desbalanceado: NÃO APLICÁVEL - trabalho bem distribuído (zocacc: 54.5%, Enzo Capellari: 45.5%)
- Ausência de Pull Requests: NÃO APLICÁVEL - foram utilizados 5 PRs corretamente:
  * PR #1: aluno-a-feature-base
  * PR #2: aluno-b-feature-fifo
  * PR #3: aluno-c-feature-lru
  * PR #4: aluno-d-feature-simulation
  * PR #5: aluno-d-feature-simulation (atualização)
- Falha de Integração Entre Módulos: NÃO APLICÁVEL - todos os módulos estão perfeitamente integrados
- Fonte dos textos: Não há demérito pois não foi identificada a fonte, mas o requisito permite isso

NOTA FINAL: 9

Cálculo: 10 (nota base) - 1.0 (documentação incompleta) = 9.0

OBSERVAÇÕES:

PONTOS FORTES DO PROJETO:

1. Estrutura de código exemplar:
   - Separação clara de responsabilidades em módulos (core/, algorithms/, simulation/)
   - Arquivo de configuração centralizado (core/config/settings.py)
   - Testes unitários implementados (core/tests/)
   - Código bem documentado com docstrings detalhadas

2. Implementação de algoritmo extra (MRU - Most Recently Used):
   - algorithms/mru_cache.py implementa o algoritmo MRU corretamente
   - MRU é um algoritmo diferente dos básicos (FIFO, LRU, LFU)
   - Demonstra conhecimento além do conteúdo básico da disciplina
   - Este foi o diferencial que elevou a nota base de 8 para 9

3. Modo de simulação robusto e completo:
   - Três distribuições de acesso implementadas corretamente:
     * Uniforme: distribuição aleatória pura (random_generators.py:27-37)
     * Poisson: distribuição de Poisson com lambda configurável (random_generators.py:39-62)
     * Ponderada: textos 30-40 com 43% de probabilidade (random_generators.py:64-91)
   - Simula 3 usuários por algoritmo (simulator.py:130-180)
   - Cada usuário faz 200 requisições conforme especificado
   - Coleta métricas detalhadas: hits, misses, tempos de carregamento (simulator.py:53-126)
   - Barra de progresso durante simulação para feedback ao usuário (simulator.py:100-104)
   - Registra estados do cache periodicamente para análise (simulator.py:108-115)

4. Geração automática de relatórios:
   - Relatórios em formato Markdown salvos em docs/ com timestamp
   - Gráficos comparativos em PNG gerados automaticamente
   - Análise estatística completa dos resultados
   - Identificação automática do melhor algoritmo (simulator.py:242-258)

5. Funcionalidades extras implementadas:
   - Modo de configuração (-2) permite ajustar algoritmo e simular disco lento
   - Sistema atualiza automaticamente o algoritmo padrão após simulação (simulator.py:282-312)
   - Simulação de disco lento configurável para testes realistas
   - Interface clara com feedback visual (barra de progresso, mensagens informativas)

6. Fluxo Git profissional:
   - 5 Pull Requests bem organizados seguindo nomenclatura clara (aluno-x-feature-y)
   - Branches dedicadas para cada funcionalidade
   - Commits com mensagens descritivas seguindo padrão (feat: descrição)
   - Todos os merges via PR, sem commits diretos na main (exceto primeiro commit)
   - Histórico limpo e organizado

7. Qualidade dos textos:
   - 100 textos com tamanho adequado (verificados aleatoriamente: 3159 e 3077 palavras)
   - Textos bem formatados e legíveis
   - Tamanho muito superior ao mínimo exigido (1000 palavras)

8. Interface de usuário intuitiva:
   - Mensagens claras e formatadas (user_interface.py)
   - Exibe informações de cache hit/miss
   - Mostra tempo de carregamento
   - Indica qual algoritmo está ativo
   - Feedback imediato para o usuário

9. Organização de código profissional:
   - Uso de type hints (typing.Dict, typing.List, etc.)
   - Docstrings completas em todos os métodos
   - Constantes de configuração centralizadas
   - Tratamento de erros adequado (try/except)
   - Código modular e reutilizável

10. Extensibilidade:
    - Fácil adicionar novos algoritmos de cache (interface clara)
    - Fácil adicionar novas distribuições de acesso
    - Configurações centralizadas facilitam ajustes
    - Estrutura permite fácil manutenção e evolução

ÁREA DE MELHORIA:

1. Documentação de autoria:
   - Adicionar nomes completos dos desenvolvedores em ra2_main.py linha 6
   - Incluir seção "Autores" ou "Desenvolvedores" no README.md
   - Especificar responsabilidades de cada membro (quem fez o quê)

2. Identificação da fonte dos textos:
   - Embora não seja obrigatório mencionar, seria bom indicar de onde vieram os textos
   - Se forem do Project Gutenberg, mencionar isso aumenta a credibilidade
   - Se forem gerados, explicar o método de geração

CONCLUSÃO:

Este é um projeto de excelente qualidade técnica que demonstra domínio dos conceitos de sistemas operacionais, algoritmos de cache e boas práticas de engenharia de software. A implementação é completa, bem estruturada e vai além dos requisitos mínimos ao incluir um algoritmo extra (MRU), testes unitários e funcionalidades adicionais. O único problema identificado foi a falta de identificação explícita dos autores no código e README, resultando no demérito de 1 ponto. Com a correção deste detalhe, o projeto alcançaria facilmente a nota máxima.
