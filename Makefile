CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lm -lncursesw

SRC_DIR = src
OBJ_DIR = obj
TEST_DIR = tests
TEST_OBJ_DIR = $(OBJ_DIR)/tests
TEST_BIN_DIR = bin/tests
OUTPUT_DIR = output

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Separar objetos do monitor principal e do cgroup_manager
MONITOR_OBJS = $(filter-out $(OBJ_DIR)/cgroup_manager.o, $(OBJS))
CGROUP_MANAGER_OBJ = $(OBJ_DIR)/cgroup_manager.o

TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c,$(TEST_OBJ_DIR)/%.o,$(TEST_SRCS))
TEST_BINS = $(patsubst $(TEST_DIR)/%.c,$(TEST_BIN_DIR)/%,$(TEST_SRCS))

TARGET = bin/monitor
CGROUP_MANAGER = bin/cgroup_manager

.PHONY: all clean tests run_tests

all: $(TARGET) $(CGROUP_MANAGER) | $(OUTPUT_DIR)

$(TARGET): $(MONITOR_OBJS) | $(OBJ_DIR) bin
	$(CC) $(MONITOR_OBJS) -o $@ $(LDFLAGS)

$(CGROUP_MANAGER): $(CGROUP_MANAGER_OBJ) | $(OBJ_DIR) bin
	$(CC) $(CGROUP_MANAGER_OBJ) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

bin:
	mkdir -p bin

tests: $(TEST_BINS)

$(TEST_BIN_DIR)/%: $(TEST_OBJ_DIR)/%.o | $(TEST_OBJ_DIR) $(TEST_BIN_DIR)
	$(CC) $< $(OBJ_DIR)/cpu_monitor.o $(OBJ_DIR)/memory_monitor.o $(OBJ_DIR)/io_monitor.o $(OBJ_DIR)/namespace_analyzer.o $(OBJ_DIR)/cgroup_v2.o $(OBJ_DIR)/utils.o $(OBJ_DIR)/network_monitor.o -o $@ $(LDFLAGS)

$(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_OBJ_DIR):
	mkdir -p $(TEST_OBJ_DIR)

$(TEST_BIN_DIR):
	mkdir -p $(TEST_BIN_DIR)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

run_tests:
	@echo "========================================"
	@echo "  Executando Todos os Testes Unitários"
	@echo "========================================"
	@echo ""
	@test_count=0; \
	passed_count=0; \
	for test_bin in $(TEST_BINS); do \
	    test_count=$$((test_count + 1)); \
	    test_name=$$(basename $$test_bin); \
	    echo ">>> Executando $$test_name..."; \
	    echo ""; \
	    if sudo $$test_bin; then \
	        passed_count=$$((passed_count + 1)); \
	    fi; \
	    echo ""; \
	    echo "========================================"; \
	    echo ""; \
	done; \
	echo ""; \
	echo "======================================== "; \
	echo "  RESUMO GERAL DOS TESTES"; \
	echo "========================================"; \
	echo "  Total de testes: $$test_count"; \
	echo "  Testes passados: $$passed_count"; \
	echo "  Testes falhados: $$((test_count - passed_count))"; \
	if [ $$passed_count -eq $$test_count ]; then \
	    echo ""; \
	    echo "  \033[0;32m[SUCESSO]\033[0m Todos os testes passaram!"; \
	else \
	    echo ""; \
	    echo "  \033[0;31m[FALHA]\033[0m Alguns testes falharam."; \
	fi; \
	echo "========================================"

valgrind: all tests
	@echo "Starting Valgrind analysis..."
	@bash scripts/valgrind_analysis.sh

clean:
	rm -rf $(OBJ_DIR) bin $(OUTPUT_DIR)

.PHONY: all tests run_tests valgrind clean
