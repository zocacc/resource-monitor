#define _GNU_SOURCE
#include "../include/namespace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <sched.h>
#include <time.h>
#include <sys/wait.h>

// Estruturas para o relatório de namespaces do sistema
typedef struct PIDNode {
    int pid;
    struct PIDNode *next;
} PIDNode;

typedef struct NamespaceReportNode {
    unsigned long inode;
    char type[20];
    char path[256];
    PIDNode *pids;
    struct NamespaceReportNode *next;
} NamespaceReportNode;

// Função auxiliar para verificar se um diretório existe
int dir_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return 1;
    }
    return 0;
}

int get_process_namespaces(int pid, NamespaceInfo *ns_info, int max_ns) {
    char ns_path[256];
    snprintf(ns_path, sizeof(ns_path), "/proc/%d/ns", pid);

    if (!dir_exists(ns_path)) {
        // Processo não existe - retorna -1 silenciosamente (não imprime erro)
        // Isso é normal durante varredura do sistema quando processos terminam
        return -1;
    }

    DIR *d = opendir(ns_path);
    if (d == NULL) {
        // Apenas imprime erro se for problema de permissão
        if (errno == EACCES) {
            fprintf(stderr, "Aviso: Sem permissão para acessar namespaces do PID %d.\n", pid);
        }
        // Outros erros são silenciosos (processo pode ter terminado)
        return -1;
    }

    struct dirent *dir;
    int count = 0;
    while ((dir = readdir(d)) != NULL && count < max_ns) {
        if (dir->d_type == DT_LNK) {
            char link_path[512];
            char real_path[512] = {0};
            struct stat sb;

            snprintf(link_path, sizeof(link_path), "%s/%s", ns_path, dir->d_name);
            if (stat(link_path, &sb) == 0) {
                strcpy(ns_info[count].type, dir->d_name);
                ns_info[count].inode = sb.st_ino;
                readlink(link_path, real_path, sizeof(real_path) - 1);
                strncpy(ns_info[count].path, real_path, sizeof(ns_info[count].path) - 1);
                count++;
            }
        }
    }
    closedir(d);
    return count;
}

void list_process_namespaces(int pid) {
    NamespaceInfo ns_info[10];
    int ns_count = get_process_namespaces(pid, ns_info, 10);

    if (ns_count < 0) {
        fprintf(stderr, "Erro: Não foi possível acessar namespaces do PID %d.\n", pid);
        fprintf(stderr, "Verifique se o processo existe ou execute com 'sudo' para processos do sistema.\n");
        return;
    }
    
    if (ns_count > 0) {
        printf("Namespaces para o PID %d:\n", pid);
        for (int i = 0; i < ns_count; i++) {
            printf("  - Tipo: %-10s | Inode: %-15lu | Path: %s\n", ns_info[i].type, ns_info[i].inode, ns_info[i].path);
        }
    }
}

void compare_process_namespaces(int pid1, int pid2) {
    NamespaceInfo ns1[10], ns2[10];
    int count1 = get_process_namespaces(pid1, ns1, 10);
    int count2 = get_process_namespaces(pid2, ns2, 10);

    if (count1 < 0 || count2 < 0) {
        return; // Erro já foi impresso
    }

    printf("\n");
    printf("===============================================================================\n");
    printf("                 COMPARACAO DE NAMESPACES ENTRE PROCESSOS\n");
    printf("===============================================================================\n");
    printf("  PID 1: %d\n", pid1);
    printf("  PID 2: %d\n", pid2);
    printf("===============================================================================\n");
    printf("\n");
    
    int shared_count = 0;
    int different_count = 0;
    
    for (int i = 0; i < count1; i++) {
        int found = 0;
        for (int j = 0; j < count2; j++) {
            if (strcmp(ns1[i].type, ns2[j].type) == 0) {
                if (ns1[i].inode == ns2[j].inode) {
                    printf("  [=] %-18s : COMPARTILHADO  (inode: %lu)\n", 
                           ns1[i].type, ns1[i].inode);
                    shared_count++;
                } else {
                    printf("  [X] %-18s : DIFERENTE      (PID1: %lu / PID2: %lu)\n", 
                           ns1[i].type, ns1[i].inode, ns2[j].inode);
                    different_count++;
                }
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("  [?] %-18s : UNICO NO PID %d (inode: %lu)\n", 
                   ns1[i].type, pid1, ns1[i].inode);
        }
    }
    
    printf("\n");
    printf("-------------------------------------------------------------------------------\n");
    printf("RESUMO:\n");
    printf("  Namespaces compartilhados: %d\n", shared_count);
    printf("  Namespaces isolados:       %d\n", different_count);
    printf("  Nivel de isolamento:       ");
    
    if (different_count == 0) {
        printf("NENHUM (processos no mesmo contexto)\n");
    } else if (different_count < 3) {
        printf("BAIXO (%d/%d namespaces isolados)\n", different_count, count1);
    } else if (different_count < 6) {
        printf("MEDIO (%d/%d namespaces isolados)\n", different_count, count1);
    } else {
        printf("ALTO (%d/%d namespaces isolados)\n", different_count, count1);
    }
    printf("===============================================================================\n");
    printf("\n");
}

void find_processes_in_namespace(const char *ns_path) {
    struct stat ns_stat;
    if (stat(ns_path, &ns_stat) != 0) {
        perror("Erro ao obter informações do arquivo de namespace");
        return;
    }

    printf("Processos no namespace com inode %lu (path: %s):\n", ns_stat.st_ino, ns_path);

    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("Não foi possível abrir /proc");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR && isdigit(*entry->d_name)) {
            int pid = atoi(entry->d_name);
            NamespaceInfo ns_info[10];
            int ns_count = get_process_namespaces(pid, ns_info, 10);
            for (int i = 0; i < ns_count; i++) {
                if (ns_info[i].inode == ns_stat.st_ino) {
                    printf("  - PID: %d (Tipo: %s)\n", pid, ns_info[i].type);
                    break; 
                }
            }
        }
    }
    closedir(proc_dir);
}

NamespaceReportNode* add_ns_report_node(NamespaceReportNode **head, const NamespaceInfo *ns, int pid) {
    NamespaceReportNode *current = *head;
    while (current != NULL) {
        if (current->inode == ns->inode) {
            PIDNode *pid_node = malloc(sizeof(PIDNode));
            pid_node->pid = pid;
            pid_node->next = current->pids;
            current->pids = pid_node;
            return current;
        }
        current = current->next;
    }

    NamespaceReportNode *new_node = malloc(sizeof(NamespaceReportNode));
    strcpy(new_node->type, ns->type);
    strcpy(new_node->path, ns->path);
    new_node->inode = ns->inode;
    new_node->next = *head;
    new_node->pids = malloc(sizeof(PIDNode));
    new_node->pids->pid = pid;
    new_node->pids->next = NULL;
    *head = new_node;
    return new_node;
}

void generate_system_namespace_report(const char *filename) {
    NamespaceReportNode *report_head = NULL;
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("Não foi possível abrir /proc");
        return;
    }

    printf("Analisando processos do sistema...\n");
    
    int total_processes = 0;
    int analyzed_processes = 0;

    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR && isdigit(*entry->d_name)) {
            total_processes++;
            int pid = atoi(entry->d_name);
            NamespaceInfo ns_info[10];
            int ns_count = get_process_namespaces(pid, ns_info, 10);
            
            if (ns_count > 0) {
                analyzed_processes++;
                for (int i = 0; i < ns_count; i++) {
                    add_ns_report_node(&report_head, &ns_info[i], pid);
                }
            }
            // Processos que retornam -1 são ignorados silenciosamente
        }
    }
    closedir(proc_dir);

    printf("Processos encontrados: %d\n", total_processes);
    printf("Processos analisados: %d\n", analyzed_processes);
    printf("Gerando relatório JSON...\n");

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Não foi possível abrir o arquivo para escrita");
        return;
    }

    fprintf(fp, "[\n");
    NamespaceReportNode *current = report_head;
    int ns_count = 0;
    while (current != NULL) {
        ns_count++;
        fprintf(fp, "  {\n");
        fprintf(fp, "    \"type\": \"%s\",\n", current->type);
        fprintf(fp, "    \"inode\": %lu,\n", current->inode);
        fprintf(fp, "    \"path\": \"%s\",\n", current->path);
        fprintf(fp, "    \"pids\": [");
        PIDNode *pid_node = current->pids;
        while (pid_node != NULL) {
            fprintf(fp, "%d%s", pid_node->pid, pid_node->next ? ", " : "");
            pid_node = pid_node->next;
        }
        fprintf(fp, "]\n");
        fprintf(fp, "  }%s\n", current->next ? "," : "");
        current = current->next;
    }
    fprintf(fp, "]\n");
    fclose(fp);

    printf("Namespaces únicos encontrados: %d\n", ns_count);
    printf("\n✓ Relatório salvo em: %s\n", filename);

    // Liberar memória
    current = report_head;
    while (current != NULL) {
        PIDNode *pid_node = current->pids;
        while (pid_node != NULL) {
            PIDNode *tmp_pid = pid_node;
            pid_node = pid_node->next;
            free(tmp_pid);
        }
        NamespaceReportNode *tmp_ns = current;
        current = current->next;
        free(tmp_ns);
    }
}

void measure_namespace_creation_overhead() {
    struct timespec start, end;
    long overhead_ns;
    int status;

    clock_gettime(CLOCK_MONOTONIC, &start);

    pid_t pid = fork();
    if (pid == 0) {
        // Processo filho: cria um novo namespace de rede
        if (unshare(CLONE_NEWNET) != 0) {
            perror("Falha ao criar novo namespace de rede");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    } else if (pid > 0) {
        // Processo pai: aguarda o filho terminar
        waitpid(pid, &status, 0);
        clock_gettime(CLOCK_MONOTONIC, &end);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            overhead_ns = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
            printf("Overhead de criação de namespace de rede: %ld µs\n", overhead_ns / 1000);
        } else {
            fprintf(stderr, "Processo filho falhou em criar o namespace.\n");
        }
    } else {
        perror("Falha no fork");
    }
}
