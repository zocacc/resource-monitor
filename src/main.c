#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>
#include "../include/monitor.h"
#include "../include/namespace.h"
#include "../include/utils.h"
#include "../include/network.h"
#include "../include/monitor_tui.h"
#include "../include/experiments.h"
#include "../include/experiment_overhead.h"
#include "../include/experiment_cpu_throttling.h"
#include "../include/experiment_memory_limit.h"
#include "../include/experiment_io_limit.h"
#include "../include/cgroup.h"
#include "../include/process_monitor.h"

void print_usage(const char *prog_name) {
    printf("Uso: %s <comando> [opções]\n\n", prog_name);
    printf("Comandos:\n");
    printf("  menu                                        - Menu interativo principal\n");
    printf("  monitor <pid> <intervalo_s> <duracao_s>     - Monitora um processo\n");
    printf("  process <pid> <intervalo_s> <duracao_s> <formato> - Monitora processo com exportação (json/csv)\n");
    printf("  tui <pid> [intervalo_s] [duracao_s]         - Interface TUI (modo interativo ou tempo determinado)\n");
    printf("  namespace list <pid>                        - Lista os namespaces de um processo\n");
    printf("  namespace compare <pid1> <pid2>             - Compara namespaces de dois processos\n");
    printf("  namespace find <caminho_ns>                 - Encontra processos em um namespace\n");
    printf("  namespace report                            - Gera relatório de namespaces em JSON\n");
    printf("  namespace overhead                          - Mede overhead de criação de namespace\n");
    printf("  experiment <1-5>                            - Executa experimento específico\n");
    printf("                                                1=Overhead, 2=Namespaces, 3=CPU,\n");
    printf("                                                4=Memory, 5=IO\n");
    printf("\n");
    printf("Exemplos:\n");
    printf("  %s process 1234 5 60 json     - Monitora PID 1234, coleta a cada 5s por 60s, exporta JSON\n", prog_name);
    printf("  %s process 1234 2 30 csv      - Monitora PID 1234, coleta a cada 2s por 30s, exporta CSV\n", prog_name);
    printf("\n");
    printf("Nota: Para gerenciar cgroups, use o programa 'cgroup_manager'\n");
    printf("\n");
}

void show_interactive_menu() {
    printf("\n");
    printf("===============================================================\n");
    printf("                                                               \n");
    printf("         RESOURCE MONITOR - MENU PRINCIPAL                     \n");
    printf("          Containers e Recursos - RA3                          \n");
    printf("                                                               \n");
    printf("===============================================================\n");
    printf("\n");
    printf("Escolha uma funcionalidade:\n\n");
    printf("  1) Resource Monitor\n");
    printf("  2) Namespace Analyzer\n");
    printf("  3) Control Group Manager\n");
    printf("  4) Experimentos (5 experimentos independentes)\n");
    printf("  0) Sair\n");
    printf("\n");
    printf("Opção: ");
}

void show_experiments_menu() {
    printf("\n");
    printf("===============================================================\n");
    printf("                  MENU DE EXPERIMENTOS                         \n");
    printf("===============================================================\n");
    printf("\n");
    printf("  1) Experimento 1: Overhead de Monitoramento\n");
    printf("  2) Experimento 2: Isolamento via Namespaces\n");
    printf("  3) Experimento 3: CPU Throttling com Cgroups\n");
    printf("  4) Experimento 4: Limite de Memória com Cgroups\n");
    printf("  5) Experimento 5: Limite de I/O com Cgroups\n");
    printf("  6) Executar TODOS os experimentos\n");
    printf("  7) Gerar visualizações (gráficos de TODOS os experimentos)\n");
    printf("  0) Voltar ao menu principal\n");
    printf("\n");
    printf("Opção: ");
}

void show_cgroup_menu() {
    printf("\n=== Control Group Manager (v2) ===\n\n");
    printf("  1) Listar todos os cgroups do sistema\n");
    printf("  2) Listar PIDs de um cgroup\n");
    printf("  3) Ler métricas de um cgroup (CPU/Memória/I/O)\n");
    printf("  4) Criar novo cgroup experimental\n");
    printf("  5) Remover cgroup experimental\n");
    printf("  6) Mover processo para cgroup\n");
    printf("  7) Aplicar limites de recursos (CPU/Memória)\n");
    printf("  8) Gerar relatório de utilização\n");
    printf("  0) Voltar ao menu principal\n\n");
    printf("Opção: ");
}

void list_all_cgroups_interactive() {
    printf("\n=== Listando Cgroups do Sistema ===\n\n");
    
    system("find /sys/fs/cgroup -maxdepth 2 -type d -name 'cgroup.*' -o -type d ! -name '*.slice' 2>/dev/null | grep -v '^/sys/fs/cgroup$' | sort");
    
    printf("\nCgroups principais:\n");
    system("ls -1d /sys/fs/cgroup/*/ 2>/dev/null | xargs -n1 basename | head -20");
}

void list_cgroup_pids(const char *cgroup_name) {
    char path[512];
    snprintf(path, sizeof(path), "/sys/fs/cgroup/%s/cgroup.procs", cgroup_name);
    
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Erro ao abrir %s: %s\n", path, strerror(errno));
        fprintf(stderr, "Verifique se o cgroup '%s' existe.\n", cgroup_name);
        return;
    }
    
    printf("\nProcessos no cgroup '%s':\n", cgroup_name);
    printf("%-10s | %-30s | %-10s\n", "PID", "COMANDO", "USUÁRIO");
    printf("--------------------------------------------------------------\n");
    
    int pid;
    int count = 0;
    while (fscanf(f, "%d", &pid) == 1) {
        char cmd[256] = "";
        char user[256] = "";
        
        // Ler comando do processo
        char cmd_path[512];
        snprintf(cmd_path, sizeof(cmd_path), "/proc/%d/comm", pid);
        FILE *fc = fopen(cmd_path, "r");
        if (fc) {
            fgets(cmd, sizeof(cmd), fc);
            cmd[strcspn(cmd, "\n")] = 0;
            fclose(fc);
        }
        
        // Ler usuário do processo
        struct stat st;
        snprintf(cmd_path, sizeof(cmd_path), "/proc/%d", pid);
        if (stat(cmd_path, &st) == 0) {
            snprintf(user, sizeof(user), "%d", st.st_uid);
        }
        
        printf("%-10d | %-30s | %-10s\n", pid, cmd, user);
        count++;
    }
    
    fclose(f);
    printf("\nTotal: %d processo(s)\n", count);
}

void run_interactive_menu() {
    int choice = 0;
    
    while (1) {
        show_interactive_menu();
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("\nOpção inválida!\n");
            continue;
        }
        while (getchar() != '\n');
        
        switch (choice) {
            case 1: {
                // Submenu Resource Monitor
                int rm_choice = 0;
                while (1) {
                    printf("\n===============================================================\n");
                    printf("                  Resource Monitor                             \n");
                    printf("===============================================================\n\n");
                    printf("Escolha o modo de monitoramento:\n\n");
                    printf("  1) TUI - Interface em Tempo Real\n");
                    printf("  2) Coleta Detalhada com Exportação (CSV/JSON)\n");
                    printf("  0) Voltar ao menu principal\n");
                    printf("\nOpção: ");
                    
                    if (scanf("%d", &rm_choice) != 1) {
                        while (getchar() != '\n');
                        printf("\nOpção inválida!\n");
                        continue;
                    }
                    while (getchar() != '\n');
                    
                    if (rm_choice == 0) {
                        break;
                    }
                    
                    switch (rm_choice) {
                        case 1: {
                            printf("\n--- TUI - Interface em Tempo Real ---\n");
                            printf("Digite o PID do processo para monitorar (ou 0 para cancelar): ");
                            int pid = 0;
                            if (scanf("%d", &pid) != 1 || pid <= 0) {
                                while (getchar() != '\n');
                                printf("Cancelado.\n");
                                break;
                            }
                            while (getchar() != '\n');
                            
                            printf("\nIniciando TUI para PID %d...\n", pid);
                            printf("Pressione 'q' para sair, 'h' para ajuda\n\n");
                            sleep(1);
                            run_tui(pid, 1, 0);
                            break;
                        }
                        
                        case 2: {
                            monitor_process_interactive();
                            break;
                        }
                        
                        default:
                            printf("\nOpção inválida!\n");
                            break;
                    }
                }
                break;
            }
            
            case 2: {
                int ns_choice = 0;
                while (1) {
                    printf("\n===============================================================\n");
                    printf("              Namespace Analyzer                               \n");
                    printf("===============================================================\n\n");
                    printf("Escolha uma funcionalidade:\n\n");
                    printf("  1) Listar namespaces de um processo\n");
                    printf("  2) Comparar namespaces entre dois processos\n");
                    printf("  3) Encontrar processos em um namespace específico\n");
                    printf("  4) Gerar relatório de todos os namespaces do sistema\n");
                    printf("  5) Medir overhead de criação de namespace\n");
                    printf("  0) Voltar ao menu principal\n");
                    printf("\nOpção: ");
                    
                    if (scanf("%d", &ns_choice) != 1) {
                        while (getchar() != '\n');
                        printf("\nOpção inválida!\n");
                        continue;
                    }
                    while (getchar() != '\n');
                    
                    if (ns_choice == 0) {
                        break;
                    }
                    
                    switch (ns_choice) {
                        case 1: {
                            printf("\n--- Listar Namespaces de um Processo ---\n");
                            printf("Digite o PID do processo: ");
                            int pid = 0;
                            if (scanf("%d", &pid) != 1 || pid <= 0) {
                                while (getchar() != '\n');
                                printf("PID inválido.\n");
                                break;
                            }
                            while (getchar() != '\n');
                            
                            printf("\n");
                            list_process_namespaces(pid);
                            printf("\nPressione ENTER para continuar...");
                            getchar();
                            break;
                        }
                        
                        case 2: {
                            printf("\n--- Comparar Namespaces entre Processos ---\n");
                            printf("Digite o PID do primeiro processo: ");
                            int pid1 = 0;
                            if (scanf("%d", &pid1) != 1 || pid1 <= 0) {
                                while (getchar() != '\n');
                                printf("PID inválido.\n");
                                break;
                            }
                            while (getchar() != '\n');
                            
                            printf("Digite o PID do segundo processo: ");
                            int pid2 = 0;
                            if (scanf("%d", &pid2) != 1 || pid2 <= 0) {
                                while (getchar() != '\n');
                                printf("PID inválido.\n");
                                break;
                            }
                            while (getchar() != '\n');
                            
                            compare_process_namespaces(pid1, pid2);
                            printf("\nPressione ENTER para continuar...");
                            getchar();
                            break;
                        }
                        
                        case 3: {
                            printf("\n--- Encontrar Processos em um Namespace ---\n");
                            printf("Digite o caminho do namespace (ex: /proc/1/ns/pid): ");
                            char ns_path[256];
                            if (fgets(ns_path, sizeof(ns_path), stdin) == NULL) {
                                printf("Entrada inválida.\n");
                                break;
                            }
                            ns_path[strcspn(ns_path, "\n")] = 0; // Remove newline
                            
                            printf("\n");
                            find_processes_in_namespace(ns_path);
                            printf("\nPressione ENTER para continuar...");
                            getchar();
                            break;
                        }
                        
                        case 4: {
                            printf("\n--- Gerar Relatório do Sistema ---\n");
                            printf("Gerando relatório de todos os namespaces do sistema...\n\n");
                            generate_system_namespace_report("output/namespace_report.json");
                            printf("\nPressione ENTER para continuar...");
                            getchar();
                            break;
                        }
                        
                        case 5: {
                            printf("\n--- Medir Overhead de Criação de Namespace ---\n\n");
                            measure_namespace_creation_overhead();
                            printf("\nPressione ENTER para continuar...");
                            getchar();
                            break;
                        }
                        
                        default:
                            printf("\nOpção inválida!\n");
                            sleep(1);
                            break;
                    }
                }
                break;
            }
            
            case 3: {
                if (geteuid() != 0) {
                    printf("\n===============================================================\n");
                    printf("              Control Group Manager                            \n");
                    printf("===============================================================\n\n");
                    printf("ATENCAO: Gerenciamento de cgroups requer privilegios root!\n\n");
                    printf("Para acessar todas as funcionalidades, execute:\n");
                    printf("  sudo ./bin/monitor menu\n\n");
                    printf("Pressione ENTER para continuar...");
                    getchar();
                    break;
                }
                
                int cg_choice = 0;
                while (1) {
                    show_cgroup_menu();
                    if (scanf("%d", &cg_choice) != 1) {
                        while (getchar() != '\n');
                        printf("\nOpção inválida!\n");
                        continue;
                    }
                    while (getchar() != '\n');
                    
                    if (cg_choice == 0) {
                        break;
                    }
                    
                    char cgroup_name[256];
                    
                    switch (cg_choice) {
                        case 1: {
                            list_all_cgroups_interactive();
                            printf("\nPressione ENTER para continuar...");
                            getchar();
                            break;
                        }
                        
                        case 2: {
                            printf("\n--- Listar PIDs de um Cgroup ---\n");
                            printf("Digite o nome do cgroup: ");
                            if (fgets(cgroup_name, sizeof(cgroup_name), stdin) == NULL) {
                                printf("Entrada inválida.\n");
                                break;
                            }
                            cgroup_name[strcspn(cgroup_name, "\n")] = 0;
                            
                            list_cgroup_pids(cgroup_name);
                            printf("\nPressione ENTER para continuar...");
                            getchar();
                            break;
                        }
                        
                        case 3: {
                            printf("\n--- Ler Métricas de Recursos ---\n");
                            printf("Digite o nome do cgroup: ");
                            if (fgets(cgroup_name, sizeof(cgroup_name), stdin) == NULL) {
                                printf("Entrada inválida.\n");
                                break;
                            }
                            cgroup_name[strcspn(cgroup_name, "\n")] = 0;
                            
                            printf("\n");
                            printf("CPU Statistics:\n");
                            read_cpu_stat_v2(cgroup_name);
                            
                            printf("\n");
                            printf("Memory Statistics:\n");
                            read_memory_stat_v2(cgroup_name);
                            
                            printf("\n");
                            printf("I/O Statistics:\n");
                            read_io_stat_v2(cgroup_name);
                            
                            printf("\nPressione ENTER para continuar...");
                            getchar();
                            break;
                        }
                        
                        case 4: {
                            printf("\n--- Criar Novo Cgroup ---\n");
                            printf("Digite o nome do novo cgroup: ");
                            if (fgets(cgroup_name, sizeof(cgroup_name), stdin) == NULL) {
                                printf("Entrada inválida.\n");
                                break;
                            }
                            cgroup_name[strcspn(cgroup_name, "\n")] = 0;
                            
                            char path[512];
                            snprintf(path, sizeof(path), "/sys/fs/cgroup/%s", cgroup_name);
                            
                            if (mkdir(path, 0755) == 0) {
                                printf("\n[OK] Cgroup '%s' criado com sucesso!\n", cgroup_name);
                                printf("Caminho: %s\n", path);
                            } else {
                                fprintf(stderr, "\n[ERRO] Erro ao criar cgroup: %s\n", strerror(errno));
                            }
                            
                            printf("\nPressione ENTER para continuar...");
                            getchar();
                            break;
                        }
                        
                        case 5: {
                            printf("\n--- Remover Cgroup ---\n");
                            printf("Digite o nome do cgroup para remover: ");
                            if (fgets(cgroup_name, sizeof(cgroup_name), stdin) == NULL) {
                                printf("Entrada inválida.\n");
                                break;
                            }
                            cgroup_name[strcspn(cgroup_name, "\n")] = 0;
                            
                            printf("\nATENCAO: Isso removera o cgroup '%s'\n", cgroup_name);
                            printf("Confirma? (s/n): ");
                            
                            char confirm[10];
                            if (fgets(confirm, sizeof(confirm), stdin) == NULL) {
                                break;
                            }
                            
                            if (confirm[0] == 's' || confirm[0] == 'S') {
                                char path[512];
                                snprintf(path, sizeof(path), "/sys/fs/cgroup/%s", cgroup_name);
                                
                                if (rmdir(path) == 0) {
                                    printf("\n[OK] Cgroup '%s' removido com sucesso!\n", cgroup_name);
                                } else {
                                    fprintf(stderr, "\n[ERRO] Erro ao remover cgroup: %s\n", strerror(errno));
                                    fprintf(stderr, "Certifique-se de que o cgroup está vazio.\n");
                                }
                            } else {
                                printf("\nOperação cancelada.\n");
                            }
                            
                            printf("\nPressione ENTER para continuar...");
                            getchar();
                            break;
                        }
                        
                        case 6: {
                            printf("\n--- Mover Processo para Cgroup ---\n");
                            printf("Digite o nome do cgroup: ");
                            if (fgets(cgroup_name, sizeof(cgroup_name), stdin) == NULL) {
                                printf("Entrada inválida.\n");
                                break;
                            }
                            cgroup_name[strcspn(cgroup_name, "\n")] = 0;
                            
                            printf("Digite o PID do processo: ");
                            int pid = 0;
                            if (scanf("%d", &pid) != 1 || pid <= 0) {
                                while (getchar() != '\n');
                                printf("PID inválido.\n");
                                break;
                            }
                            while (getchar() != '\n');
                            
                            if (add_process_to_cgroup(pid, "", cgroup_name) == 0) {
                                printf("\n[OK] Processo %d movido para cgroup '%s'\n", pid, cgroup_name);
                            } else {
                                fprintf(stderr, "\n[ERRO] Erro ao mover processo\n");
                            }
                            
                            printf("\nPressione ENTER para continuar...");
                            getchar();
                            break;
                        }
                        
                        case 7: {
                            printf("\n--- Aplicar Limites de Recursos ---\n");
                            printf("Digite o nome do cgroup: ");
                            if (fgets(cgroup_name, sizeof(cgroup_name), stdin) == NULL) {
                                printf("Entrada inválida.\n");
                                break;
                            }
                            cgroup_name[strcspn(cgroup_name, "\n")] = 0;
                            
                            int limit_choice = 0;
                            printf("\nEscolha o tipo de limite:\n");
                            printf("  1) Limite de CPU\n");
                            printf("  2) Limite de Memória\n");
                            printf("  3) Ambos (CPU + Memória)\n");
                            printf("Opção: ");
                            
                            if (scanf("%d", &limit_choice) != 1) {
                                while (getchar() != '\n');
                                printf("Opção inválida.\n");
                                break;
                            }
                            while (getchar() != '\n');
                            
                            if (limit_choice == 1 || limit_choice == 3) {
                                printf("\nDigite a porcentagem de CPU (1-100): ");
                                int cpu_percent = 0;
                                if (scanf("%d", &cpu_percent) != 1 || cpu_percent < 1 || cpu_percent > 100) {
                                    while (getchar() != '\n');
                                    printf("Porcentagem inválida.\n");
                                    break;
                                }
                                while (getchar() != '\n');
                                
                                int period_us = 100000;
                                int quota_us = (cpu_percent * period_us) / 100;
                                
                                if (set_cpu_quota(cgroup_name, period_us, quota_us) == 0) {
                                    printf("[OK] Limite de CPU aplicado: %d%% (quota=%d us, period=%d us)\n", 
                                           cpu_percent, quota_us, period_us);
                                } else {
                                    fprintf(stderr, "[ERRO] Erro ao aplicar limite de CPU\n");
                                }
                            }
                            
                            if (limit_choice == 2 || limit_choice == 3) {
                                printf("\nDigite o limite de memória em MB: ");
                                long long mem_mb = 0;
                                if (scanf("%lld", &mem_mb) != 1 || mem_mb <= 0) {
                                    while (getchar() != '\n');
                                    printf("Valor inválido.\n");
                                    break;
                                }
                                while (getchar() != '\n');
                                
                                long long mem_bytes = mem_mb * 1024 * 1024;
                                
                                if (set_memory_limit(cgroup_name, mem_bytes) == 0) {
                                    printf("[OK] Limite de memoria aplicado: %lld MB (%lld bytes)\n", 
                                           mem_mb, mem_bytes);
                                } else {
                                    fprintf(stderr, "[ERRO] Erro ao aplicar limite de memoria\n");
                                }
                            }
                            
                            printf("\nPressione ENTER para continuar...");
                            getchar();
                            break;
                        }
                        
                        case 8: {
                            printf("\n--- Gerar Relatório de Utilização ---\n");
                            printf("Digite o nome do cgroup: ");
                            if (fgets(cgroup_name, sizeof(cgroup_name), stdin) == NULL) {
                                printf("Entrada inválida.\n");
                                break;
                            }
                            cgroup_name[strcspn(cgroup_name, "\n")] = 0;
                            
                            char filename[512];
                            snprintf(filename, sizeof(filename), "output/cgroup_%s_report.json", cgroup_name);
                            
                            export_cgroup_info_to_json("", cgroup_name, filename);
                            printf("\n[OK] Relatorio salvo em: %s\n", filename);
                            
                            printf("\nPressione ENTER para continuar...");
                            getchar();
                            break;
                        }
                        
                        default:
                            printf("\nOpção inválida!\n");
                            sleep(1);
                            break;
                    }
                }
                break;
            }
            
            case 4: {
                int exp_choice = 0;
                while (1) {
                    show_experiments_menu();
                    if (scanf("%d", &exp_choice) != 1) {
                        while (getchar() != '\n');
                        printf("\nOpção inválida!\n");
                        continue;
                    }
                    while (getchar() != '\n');
                    
                    if (exp_choice == 0) {
                        break;
                    }
                    
                    char output_path[256];
                    
                    if (exp_choice == 7) {
                        // Gerar visualizações de todos os experimentos
                        printf("\n===============================================================\n");
                        printf("Gerando visualizacoes de TODOS os experimentos...\n");
                        printf("===============================================================\n\n");
                        
                        printf("Verificando arquivos de dados...\n");
                        system("ls -lh output/experiment*.csv output/experiments/exp*.json 2>/dev/null | tail -10");
                        
                        printf("\nPreparando diretório de saída...\n");
                        system("mkdir -p output/graphs 2>/dev/null");
                        
                        printf("\nExecutando visualize.py...\n\n");
                        int ret = system("venv/bin/python scripts/visualize.py --experiments output/graphs 2>&1");
                        
                        if (ret == 0) {
                            printf("\n===============================================================\n");
                            printf("Visualizacoes geradas com sucesso!\n");
                            printf("===============================================================\n\n");
                            printf("Gráficos salvos em: output/graphs/\n\n");
                            printf("Listando gráficos gerados:\n");
                            system("ls -lh output/graphs/*.png 2>/dev/null");
                        } else {
                            printf("\nAviso: Não foi possível gerar todas as visualizações.\n");
                            printf("Certifique-se de que:\n");
                            printf("  1. Python3 está instalado\n");
                            printf("  2. matplotlib e numpy estão instalados (venv/bin/pip install matplotlib numpy)\n");
                            printf("  3. Os experimentos foram executados\n");
                            printf("\nTente manualmente: venv/bin/python scripts/visualize.py --experiments output/graphs\n");
                        }
                        
                        printf("\nPressione ENTER para continuar...");
                        getchar();
                        
                    } else if (exp_choice == 6) {
                        // Executar todos os experimentos
                        printf("\n===============================================================\n");
                        printf("Executando TODOS os Experimentos (1-5)\n");
                        printf("===============================================================\n\n");
                        
                        for (int i = 1; i <= 5; i++) {
                            snprintf(output_path, sizeof(output_path), 
                                     "output/experiments/exp%d_*.json", i);
                            
                            switch (i) {
                                case 1:
                                    run_experiment_overhead();
                                    break;
                                case 2:
                                    run_experiment_namespace_isolation("output/experiments/exp2_namespace_isolation.json");
                                    break;
                                case 3:
                                    run_experiment_cpu_throttling_new();
                                    break;
                                case 4:
                                    run_experiment_memory_limit_new();
                                    break;
                                case 5:
                                    run_experiment_io_limit_new();
                                    break;
                            }
                            
                            printf("\nPressione ENTER para próximo experimento...");
                            getchar();
                        }
                        
                        printf("\n===============================================================\n");
                        printf("  TODOS OS EXPERIMENTOS CONCLUIDOS!                            \n");
                        printf("===============================================================\n\n");
                        
                        // Gerar visualizações de todos os experimentos
                        printf("===============================================================\n");
                        printf("Gerando visualizacoes de TODOS os experimentos...\n");
                        printf("===============================================================\n\n");
                        
                        system("mkdir -p output/graphs 2>/dev/null");
                        int ret = system("venv/bin/python scripts/visualize.py --experiments output/graphs 2>&1 | tail -30");
                        if (ret == 0) {
                            printf("\nVisualizações salvas em: output/graphs/\n");
                        } else {
                            printf("\nAviso: Não foi possível gerar visualizações.\n");
                            printf("Execute manualmente: venv/bin/python scripts/visualize.py --experiments output/graphs\n");
                        }
                        
                        printf("\nPressione ENTER para continuar...");
                        getchar();
                        
                    } else if (exp_choice >= 1 && exp_choice <= 5) {
                        switch (exp_choice) {
                            case 1:
                                run_experiment_overhead();
                                break;
                            case 2:
                                run_experiment_namespace_isolation("output/experiments/exp2_namespace_isolation.json");
                                break;
                            case 3:
                                run_experiment_cpu_throttling_new();
                                break;
                            case 4:
                                run_experiment_memory_limit_new();
                                break;
                            case 5:
                                run_experiment_io_limit_new();
                                break;
                        }
                        
                        // Gerar visualizações automaticamente
                        printf("\n===============================================================\n");
                        printf("Gerando visualizacoes...\n");
                        printf("===============================================================\n\n");
                        
                        char vis_cmd[512];
                        const char *exp_file = NULL;
                        
                        switch (exp_choice) {
                            case 1:
                                exp_file = "output/experiment1_overhead.csv";
                                break;
                            case 2:
                                exp_file = "output/experiments/exp2_namespace_isolation.json";
                                break;
                            case 3:
                                exp_file = "output/experiment3_cpu_throttling.csv";
                                break;
                            case 4:
                                exp_file = "output/experiment4_memory_limit.csv";
                                break;
                            case 5:
                                exp_file = "output/experiment5_io_limit.csv";
                                break;
                        }
                        
                        if (exp_file) {
                            snprintf(vis_cmd, sizeof(vis_cmd), 
                                    "venv/bin/python scripts/visualize.py %s output/graphs 2>&1 | tail -20", 
                                    exp_file);
                            int ret = system(vis_cmd);
                            if (ret == 0) {
                                printf("\nVisualizações salvas em: output/graphs/\n");
                            } else {
                                printf("\nAviso: Não foi possível gerar visualizações.\n");
                                printf("Execute manualmente: venv/bin/python scripts/visualize.py %s output/graphs\n", exp_file);
                            }
                        }
                        
                        printf("\nPressione ENTER para continuar...");
                        getchar();
                    } else {
                        printf("\nOpção inválida!\n");
                    }
                }
                break;
            }
            
            case 0:
                printf("\nSaindo...\n\n");
                return;
                
            default:
                printf("\nOpção inválida!\n");
                break;
        }
    }
}


void run_monitor(int pid, int interval, int duration) {
    int num_samples = duration / interval;
    if (num_samples <= 0) {
        fprintf(stderr, "Erro: Duração deve ser maior que o intervalo.\n");
        return;
    }

    ResourceData *history = malloc(num_samples * sizeof(ResourceData));
    if (history == NULL) {
        perror("Falha ao alocar memória para o histórico");
        return;
    }

    ResourceData prev_data = {0};
    bool first_sample = true;
    long ticks_per_second = sysconf(_SC_CLK_TCK);

    printf("Monitorando PID %d a cada %d segundos por %d segundos...\n", pid, interval, duration);
    printf("%-8s | %-7s | %-10s | %-10s | %-12s | %-12s | %-8s\n",
           "TEMPO", "CPU%", "MEM(VSZ)", "MEM(RSS)", "IO_R_RATE", "IO_W_RATE", "USER");

    for (int i = 0; i < num_samples; i++) {
        ResourceData current_data;

        // Coleta de dados
        if (!get_cpu_data(pid, &current_data) || !get_memory_data(pid, &current_data) || !get_io_data(pid, &current_data) || !get_network_data(pid, &current_data)) {
            fprintf(stderr, "\nErro: Não foi possível ler os dados do processo %d. Ele pode ter sido encerrado.\n", pid);
            free(history);
            return;
        }

        if (first_sample) {
            first_sample = false;
            current_data.cpu_usage_percent = 0.0;
            current_data.io_read_rate = 0.0;
            current_data.io_write_rate = 0.0;
        } else {
            // Calcular deltas
            double time_delta_sec = difftime(current_data.timestamp, prev_data.timestamp);
            if (time_delta_sec <= 0) time_delta_sec = interval; // Fallback

            // CPU %
            long total_cpu_time_diff = (current_data.cpu_user + current_data.cpu_system) - (prev_data.cpu_user + prev_data.cpu_system);
            current_data.cpu_usage_percent = 100.0 * (total_cpu_time_diff / (double)ticks_per_second) / time_delta_sec;

            // I/O Rates
            current_data.io_read_rate = (current_data.io_read_bytes - prev_data.io_read_bytes) / time_delta_sec;
            current_data.io_write_rate = (current_data.io_write_bytes - prev_data.io_write_bytes) / time_delta_sec;
        }

        // Armazenar e imprimir
        history[i] = current_data;
        prev_data = current_data;

        printf("%-8ld | %-6.2f%% | %-10ld | %-10ld | %-12.2f | %-12.2f | %-8ld\n",
               current_data.timestamp,
               current_data.cpu_usage_percent,
               current_data.memory_vsz,
               current_data.memory_rss * (sysconf(_SC_PAGESIZE) / 1024),
               current_data.io_read_rate,
               current_data.io_write_rate,
               current_data.cpu_user);

        if (i < num_samples - 1) {
            sleep(interval);
        }
    }

    // Exportar dados
    export_to_json(history, num_samples, "output/monitor_output.json");

    free(history);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    char *command = argv[1];

    if (strcmp(command, "menu") == 0) {
        run_interactive_menu();
        return 0;

    } else if (strcmp(command, "monitor") == 0) {
        if (argc != 5) {
            fprintf(stderr, "Erro: 'monitor' requer PID, intervalo (s) e duração (s).\n");
            print_usage(argv[0]);
            return 1;
        }
        int pid = atoi(argv[2]);
        int interval = atoi(argv[3]);
        int duration = atoi(argv[4]);
        run_monitor(pid, interval, duration);

    } else if (strcmp(command, "process") == 0) {
        if (argc != 6) {
            fprintf(stderr, "Erro: 'process' requer PID, intervalo (s), duração (s) e formato (json/csv).\n");
            fprintf(stderr, "Uso: %s process <pid> <intervalo_s> <duracao_s> <json|csv>\n", argv[0]);
            fprintf(stderr, "Exemplo: %s process 1234 5 60 json\n", argv[0]);
            return 1;
        }
        int pid = atoi(argv[2]);
        int interval = atoi(argv[3]);
        int duration = atoi(argv[4]);
        const char *format = argv[5];
        
        if (strcmp(format, "json") != 0 && strcmp(format, "csv") != 0) {
            fprintf(stderr, "Erro: Formato inválido '%s'. Use 'json' ou 'csv'.\n", format);
            return 1;
        }
        
        monitor_process_continuous(pid, interval, duration, format);

    } else if (strcmp(command, "tui") == 0) {
        if (argc < 3 || argc > 5) {
            fprintf(stderr, "Erro: 'tui' requer PID e opcionalmente intervalo e duração.\n");
            fprintf(stderr, "Uso: %s tui <pid> [intervalo_s] [duracao_s]\n", argv[0]);
            return 1;
        }
        int pid = atoi(argv[2]);
        int interval = (argc >= 4) ? atoi(argv[3]) : 0;
        int duration = (argc >= 5) ? atoi(argv[4]) : 0;
        return run_tui(pid, interval, duration);

    } else if (strcmp(command, "namespace") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Erro: O comando 'namespace' requer um subcomando.\n");
            print_usage(argv[0]);
            return 1;
        }
        char *subcommand = argv[2];
        if (strcmp(subcommand, "list") == 0) {
            if (argc != 4) {
                fprintf(stderr, "Uso: %s namespace list <pid>\n", argv[0]);
                return 1;
            }
            list_process_namespaces(atoi(argv[3]));
        } else if (strcmp(subcommand, "compare") == 0) {
            if (argc != 5) {
                fprintf(stderr, "Uso: %s namespace compare <pid1> <pid2>\n", argv[0]);
                return 1;
            }
            compare_process_namespaces(atoi(argv[3]), atoi(argv[4]));
        } else if (strcmp(subcommand, "find") == 0) {
            if (argc != 4) {
                fprintf(stderr, "Uso: %s namespace find <caminho_ns>\n", argv[0]);
                return 1;
            }
            find_processes_in_namespace(argv[3]);
        } else if (strcmp(subcommand, "report") == 0) {
            generate_system_namespace_report("output/namespace_report.json");
        } else if (strcmp(subcommand, "overhead") == 0) {
            measure_namespace_creation_overhead();
        } else {
            fprintf(stderr, "Subcomando de namespace desconhecido: %s\n", subcommand);
            print_usage(argv[0]);
            return 1;
        }

    } else if (strcmp(command, "experiment") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Erro: O comando 'experiment' requer um número (1-5).\n");
            fprintf(stderr, "Uso: %s experiment <1-5>\n", argv[0]);
            return 1;
        }
        int exp_num = atoi(argv[2]);
        char output_path[256];
        
        switch (exp_num) {
            case 1:
                run_experiment_overhead();
                break;
            case 2:
                snprintf(output_path, sizeof(output_path), "output/experiments/exp2_namespace_isolation.json");
                run_experiment_namespace_isolation(output_path);
                break;
            case 3:
                run_experiment_cpu_throttling_new();
                break;
            case 4:
                run_experiment_memory_limit_new();
                break;
            case 5:
                run_experiment_io_limit_new();
                break;
            default:
                fprintf(stderr, "Erro: Número de experimento inválido: %d\n", exp_num);
                fprintf(stderr, "Use um número entre 1 e 5.\n");
                return 1;
        }

    } else {
        fprintf(stderr, "Comando desconhecido: %s\n", command);
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}