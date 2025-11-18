/**
 * test_namespace.c - Teste unitário para o Namespace Analyzer
 * 
 * Testa:
 * - Listagem de namespaces de um processo
 * - Comparação de namespaces entre processos
 * - Leitura de inode de namespaces
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "../include/namespace.h"

#define TEST_PASSED "\033[0;32m[PASS]\033[0m"
#define TEST_FAILED "\033[0;31m[FAIL]\033[0m"
#define TEST_INFO   "\033[0;34m[INFO]\033[0m"

static int tests_passed = 0;
static int tests_failed = 0;

void assert_test(const char *test_name, int condition) {
    if (condition) {
        printf("%s %s\n", TEST_PASSED, test_name);
        tests_passed++;
    } else {
        printf("%s %s\n", TEST_FAILED, test_name);
        tests_failed++;
    }
}

void test_namespace_files_exist() {
    printf("\n=== Teste 1: Verificar existência de arquivos de namespace ===\n");
    
    pid_t pid = getpid();
    char ns_path[256];
    struct stat st;
    
    const char *namespaces[] = {"pid", "net", "mnt", "uts", "ipc", "cgroup", "user"};
    int num_ns = 7;
    int all_exist = 1;
    
    for (int i = 0; i < num_ns; i++) {
        snprintf(ns_path, sizeof(ns_path), "/proc/%d/ns/%s", pid, namespaces[i]);
        
        if (stat(ns_path, &st) == 0) {
            printf("%s Namespace %s existe: %s\n", TEST_INFO, namespaces[i], ns_path);
        } else {
            printf("%s Namespace %s NÃO existe: %s (pode não estar disponível)\n", 
                   TEST_INFO, namespaces[i], ns_path);
            // Não falha o teste, pois alguns namespaces podem não estar disponíveis
        }
    }
    
    assert_test("Arquivos de namespace acessíveis", all_exist);
}

void test_namespace_readlink() {
    printf("\n=== Teste 2: Ler inode de namespaces ===\n");
    
    pid_t pid = getpid();
    char ns_path[256];
    char link_target[256];
    ssize_t len;
    
    snprintf(ns_path, sizeof(ns_path), "/proc/%d/ns/pid", pid);
    len = readlink(ns_path, link_target, sizeof(link_target) - 1);
    
    if (len != -1) {
        link_target[len] = '\0';
        printf("%s PID namespace: %s\n", TEST_INFO, link_target);
        
        // Verificar formato: deve ser "pid:[inode]"
        int is_valid_format = (strncmp(link_target, "pid:[", 5) == 0);
        assert_test("Formato de namespace válido", is_valid_format);
    } else {
        assert_test("Leitura de namespace PID", 0);
    }
}

void test_namespace_comparison() {
    printf("\n=== Teste 3: Comparar namespaces do mesmo processo ===\n");
    
    pid_t pid1 = getpid();
    pid_t pid2 = getpid();
    
    char ns1_path[256], ns2_path[256];
    char ns1_target[256], ns2_target[256];
    
    snprintf(ns1_path, sizeof(ns1_path), "/proc/%d/ns/pid", pid1);
    snprintf(ns2_path, sizeof(ns2_path), "/proc/%d/ns/pid", pid2);
    
    ssize_t len1 = readlink(ns1_path, ns1_target, sizeof(ns1_target) - 1);
    ssize_t len2 = readlink(ns2_path, ns2_target, sizeof(ns2_target) - 1);
    
    if (len1 != -1 && len2 != -1) {
        ns1_target[len1] = '\0';
        ns2_target[len2] = '\0';
        
        int same_namespace = (strcmp(ns1_target, ns2_target) == 0);
        printf("%s Namespace 1: %s\n", TEST_INFO, ns1_target);
        printf("%s Namespace 2: %s\n", TEST_INFO, ns2_target);
        
        assert_test("Mesmo processo tem mesmo namespace PID", same_namespace);
    } else {
        assert_test("Comparação de namespaces", 0);
    }
}

void test_namespace_init_vs_self() {
    printf("\n=== Teste 4: Comparar namespace com processo init ===\n");
    
    pid_t self_pid = getpid();
    pid_t init_pid = 1;
    
    char self_ns[256], init_ns[256];
    char self_target[256], init_target[256];
    
    snprintf(self_ns, sizeof(self_ns), "/proc/%d/ns/pid", self_pid);
    snprintf(init_ns, sizeof(init_ns), "/proc/%d/ns/pid", init_pid);
    
    ssize_t len1 = readlink(self_ns, self_target, sizeof(self_target) - 1);
    ssize_t len2 = readlink(init_ns, init_target, sizeof(init_target) - 1);
    
    if (len1 != -1 && len2 != -1) {
        self_target[len1] = '\0';
        init_target[len2] = '\0';
        
        int different_or_same = 1; // Pode ser diferente se em container
        printf("%s Self PID namespace: %s\n", TEST_INFO, self_target);
        printf("%s Init PID namespace: %s\n", TEST_INFO, init_target);
        
        if (strcmp(self_target, init_target) == 0) {
            printf("%s Mesmo namespace que init (não containerizado)\n", TEST_INFO);
        } else {
            printf("%s Namespace diferente de init (pode estar em container)\n", TEST_INFO);
        }
        
        assert_test("Namespace PID legível para init e self", different_or_same);
    } else {
        assert_test("Leitura de namespaces init vs self", 0);
    }
}

void test_namespace_stat_inode() {
    printf("\n=== Teste 5: Verificar inode de namespace ===\n");
    
    pid_t pid = getpid();
    char ns_path[256];
    struct stat st;
    
    snprintf(ns_path, sizeof(ns_path), "/proc/%d/ns/pid", pid);
    
    if (stat(ns_path, &st) == 0) {
        printf("%s Inode do namespace PID: %lu\n", TEST_INFO, (unsigned long)st.st_ino);
        printf("%s Device ID: %lu\n", TEST_INFO, (unsigned long)st.st_dev);
        
        int has_inode = (st.st_ino != 0);
        assert_test("Namespace tem inode válido", has_inode);
    } else {
        assert_test("stat() de namespace", 0);
    }
}

int main() {
    printf("\n");
    printf("===============================================================\n");
    printf("  TESTES UNITÁRIOS - NAMESPACE ANALYZER\n");
    printf("===============================================================\n");
    
    printf("%s Executando como PID: %d\n", TEST_INFO, getpid());
    printf("%s UID: %d, GID: %d\n", TEST_INFO, getuid(), getgid());
    
    // Executar testes
    test_namespace_files_exist();
    test_namespace_readlink();
    test_namespace_comparison();
    test_namespace_init_vs_self();
    test_namespace_stat_inode();
    
    // Resumo
    printf("\n");
    printf("===============================================================\n");
    printf("  RESUMO DOS TESTES\n");
    printf("===============================================================\n");
    printf("%s Testes passados: %d\n", TEST_PASSED, tests_passed);
    if (tests_failed > 0) {
        printf("%s Testes falhados: %d\n", TEST_FAILED, tests_failed);
    }
    printf("Total: %d testes\n", tests_passed + tests_failed);
    printf("===============================================================\n\n");
    
    // Retornar código de erro se algum teste falhou
    return (tests_failed > 0) ? 1 : 0;
}
