#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "../include/cgroup.h"

// ANSI color codes for output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"

// Test counter
static int tests_passed = 0;
static int tests_failed = 0;

// Helper macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("  " COLOR_GREEN "[OK]" COLOR_RESET " %s\n", message); \
            tests_passed++; \
        } else { \
            printf("  " COLOR_RED "[ERRO]" COLOR_RESET " %s\n", message); \
            tests_failed++; \
        } \
    } while(0)

#define TEST_START(name) \
    printf("\n" COLOR_BLUE "=== Teste: %s ===" COLOR_RESET "\n", name)

// Test 1: Verify cgroup v2 is mounted
void test_cgroup_v2_mounted() {
    TEST_START("Cgroup v2 Montado");
    
    struct stat st;
    int result = stat("/sys/fs/cgroup/cgroup.controllers", &st);
    
    TEST_ASSERT(result == 0, "Cgroup v2 está montado em /sys/fs/cgroup");
    TEST_ASSERT(S_ISREG(st.st_mode), "cgroup.controllers é um arquivo regular");
}

// Test 2: Check available controllers
void test_cgroup_controllers() {
    TEST_START("Controllers Disponíveis");
    
    FILE *fp = fopen("/sys/fs/cgroup/cgroup.controllers", "r");
    TEST_ASSERT(fp != NULL, "Conseguiu abrir cgroup.controllers");
    
    if (fp) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), fp)) {
            TEST_ASSERT(strstr(buffer, "cpu") != NULL, "Controller 'cpu' disponível");
            TEST_ASSERT(strstr(buffer, "memory") != NULL, "Controller 'memory' disponível");
            TEST_ASSERT(strstr(buffer, "io") != NULL, "Controller 'io' disponível");
            TEST_ASSERT(strstr(buffer, "pids") != NULL, "Controller 'pids' disponível");
        }
        fclose(fp);
    }
}

// Test 3: Test cgroup creation (requires root)
void test_cgroup_creation() {
    TEST_START("Criação de Cgroup");
    
    if (geteuid() != 0) {
        printf("  " COLOR_YELLOW "[SKIP]" COLOR_RESET " Requer privilégios root\n");
        return;
    }
    
    const char *test_cgroup = "/sys/fs/cgroup/test_monitor";
    
    // Try to create test cgroup
    int result = mkdir(test_cgroup, 0755);
    if (result == 0 || errno == EEXIST) {
        TEST_ASSERT(1, "Cgroup de teste criado/existente");
        
        // Verify directory was created
        struct stat st;
        result = stat(test_cgroup, &st);
        TEST_ASSERT(result == 0 && S_ISDIR(st.st_mode), "Diretório de cgroup existe");
        
        // Verify interface files exist
        char path[512];
        snprintf(path, sizeof(path), "%s/cgroup.procs", test_cgroup);
        result = access(path, F_OK);
        TEST_ASSERT(result == 0, "Arquivo cgroup.procs existe");
        
        snprintf(path, sizeof(path), "%s/cgroup.controllers", test_cgroup);
        result = access(path, F_OK);
        TEST_ASSERT(result == 0, "Arquivo cgroup.controllers existe");
        
        // Cleanup
        rmdir(test_cgroup);
    } else {
        TEST_ASSERT(0, "Falha ao criar cgroup de teste");
    }
}

// Test 4: Test reading cgroup limits
void test_cgroup_limits() {
    TEST_START("Leitura de Limites");
    
    // Test reading CPU max (root cgroup)
    FILE *fp = fopen("/sys/fs/cgroup/cpu.max", "r");
    if (fp) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), fp)) {
            TEST_ASSERT(strlen(buffer) > 0, "Leitura de cpu.max bem-sucedida");
        }
        fclose(fp);
    }
    
    // Test reading memory max (root cgroup)
    fp = fopen("/sys/fs/cgroup/memory.max", "r");
    if (fp) {
        char buffer[128];
        if (fgets(buffer, sizeof(buffer), fp)) {
            TEST_ASSERT(strlen(buffer) > 0, "Leitura de memory.max bem-sucedida");
        }
        fclose(fp);
    }
}

// Test 5: Test reading cgroup statistics
void test_cgroup_stats() {
    TEST_START("Leitura de Estatísticas");
    
    // Test reading CPU stats
    FILE *fp = fopen("/sys/fs/cgroup/cpu.stat", "r");
    if (fp) {
        char buffer[256];
        int found_usage = 0;
        while (fgets(buffer, sizeof(buffer), fp)) {
            if (strstr(buffer, "usage_usec")) {
                found_usage = 1;
                break;
            }
        }
        TEST_ASSERT(found_usage, "Estatística 'usage_usec' encontrada em cpu.stat");
        fclose(fp);
    }
    
    // Test reading memory stats
    fp = fopen("/sys/fs/cgroup/memory.stat", "r");
    if (fp) {
        char buffer[256];
        int found_anon = 0;
        while (fgets(buffer, sizeof(buffer), fp)) {
            if (strstr(buffer, "anon")) {
                found_anon = 1;
                break;
            }
        }
        TEST_ASSERT(found_anon, "Estatística 'anon' encontrada em memory.stat");
        fclose(fp);
    }
}

// Test 6: Test process cgroup membership
void test_process_cgroup() {
    TEST_START("Cgroup do Processo");
    
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/cgroup", getpid());
    
    FILE *fp = fopen(path, "r");
    TEST_ASSERT(fp != NULL, "Conseguiu abrir /proc/self/cgroup");
    
    if (fp) {
        char buffer[512];
        if (fgets(buffer, sizeof(buffer), fp)) {
            // Cgroup v2 format: 0::/path
            TEST_ASSERT(buffer[0] == '0', "Formato cgroup v2 detectado");
            TEST_ASSERT(strstr(buffer, "::") != NULL, "Separador :: encontrado");
        }
        fclose(fp);
    }
}

// Test 7: Test cgroup subtree control (requires root)
void test_cgroup_subtree_control() {
    TEST_START("Controle de Subtree");
    
    if (geteuid() != 0) {
        printf("  " COLOR_YELLOW "[SKIP]" COLOR_RESET " Requer privilégios root\n");
        return;
    }
    
    FILE *fp = fopen("/sys/fs/cgroup/cgroup.subtree_control", "r");
    if (fp) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), fp)) {
            TEST_ASSERT(1, "Leitura de subtree_control bem-sucedida");
        }
        fclose(fp);
    }
}

// Test 8: Test writing to cgroup.procs (requires root)
void test_cgroup_procs() {
    TEST_START("Movimentação de Processos");
    
    if (geteuid() != 0) {
        printf("  " COLOR_YELLOW "[SKIP]" COLOR_RESET " Requer privilégios root\n");
        return;
    }
    
    const char *test_cgroup = "/sys/fs/cgroup/test_monitor";
    
    // Create test cgroup
    mkdir(test_cgroup, 0755);
    
    // Try to move current process
    char procs_path[512];
    snprintf(procs_path, sizeof(procs_path), "%s/cgroup.procs", test_cgroup);
    
    FILE *fp = fopen(procs_path, "w");
    if (fp) {
        fprintf(fp, "%d\n", getpid());
        fclose(fp);
        
        // Verify move
        char path[256];
        snprintf(path, sizeof(path), "/proc/%d/cgroup", getpid());
        fp = fopen(path, "r");
        if (fp) {
            char buffer[512];
            if (fgets(buffer, sizeof(buffer), fp)) {
                TEST_ASSERT(strstr(buffer, "test_monitor") != NULL, 
                           "Processo movido para cgroup de teste");
            }
            fclose(fp);
        }
        
        // Move back to root
        fp = fopen("/sys/fs/cgroup/cgroup.procs", "w");
        if (fp) {
            fprintf(fp, "%d\n", getpid());
            fclose(fp);
        }
    }
    
    // Cleanup
    rmdir(test_cgroup);
}

int main() {
    printf("\n");
    printf("====================================\n");
    printf("  Testes de Cgroups v2\n");
    printf("====================================\n");
    
    // Run all tests
    test_cgroup_v2_mounted();
    test_cgroup_controllers();
    test_cgroup_creation();
    test_cgroup_limits();
    test_cgroup_stats();
    test_process_cgroup();
    test_cgroup_subtree_control();
    test_cgroup_procs();
    
    // Print summary
    printf("\n");
    printf("====================================\n");
    printf("  Resumo dos Testes\n");
    printf("====================================\n");
    printf("  " COLOR_GREEN "Passou: %d" COLOR_RESET "\n", tests_passed);
    printf("  " COLOR_RED "Falhou: %d" COLOR_RESET "\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("\n  " COLOR_GREEN "[SUCESSO]" COLOR_RESET " Todos os testes passaram!\n\n");
        return 0;
    } else {
        printf("\n  " COLOR_RED "[FALHA]" COLOR_RESET " Alguns testes falharam.\n\n");
        return 1;
    }
}
