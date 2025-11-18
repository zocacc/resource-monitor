#ifndef CGROUP_H
#define CGROUP_H

// Cria um novo cgroup
int create_cgroup(const char *controller, const char *group_name);

// Remove um cgroup
int remove_cgroup(const char *controller, const char *group_name);

// Define um limite de recurso para um cgroup
int set_cgroup_limit(const char *controller, const char *group_name, const char *parameter, const char *value);

// Adiciona um processo a um cgroup
int add_process_to_cgroup(int pid, const char *controller, const char *group_name);

// Lê o valor de um parâmetro de um cgroup
char *get_cgroup_value(const char *controller, const char *group_name, const char *parameter);

// Exporta informações do cgroup para JSON
void export_cgroup_info_to_json(const char *controller, const char *group_name, const char *filename);

// Funções auxiliares para configuração de limites específicos
int set_cpu_quota(const char *group_name, int period_us, int quota_us);
int set_memory_limit(const char *group_name, long long limit_bytes);

// Funções de leitura de estatísticas cgroup v2
int read_cpu_stat_v2(const char *cgroup_name);
int read_memory_stat_v2(const char *cgroup_name);
int read_pids_stat_v2(const char *cgroup_name);
int read_io_stat_v2(const char *cgroup_name);

// Funções de configuração cgroup v2
int set_pids_max_v2(const char *cgroup_name, int max_pids);

#endif // CGROUP_H
