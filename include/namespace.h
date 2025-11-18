#ifndef NAMESPACE_H
#define NAMESPACE_H

// Estrutura para representar informações de um namespace
typedef struct {
    char type[20];
    char path[256];
    unsigned long inode;
} NamespaceInfo;

// Obtém os namespaces de um processo
int get_process_namespaces(int pid, NamespaceInfo *ns_info, int max_ns);

// Lista os namespaces de um processo
void list_process_namespaces(int pid);

// Compara os namespaces de dois processos
void compare_process_namespaces(int pid1, int pid2);

// Encontra processos em um determinado namespace
void find_processes_in_namespace(const char *ns_path);

// Gera um relatório de namespaces do sistema para um arquivo JSON
void generate_system_namespace_report(const char *filename);

// Mede o overhead de criação de um novo namespace
void measure_namespace_creation_overhead();

#endif // NAMESPACE_H
