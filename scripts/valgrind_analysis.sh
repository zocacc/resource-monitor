#!/bin/bash
# valgrind_analysis.sh - Script para análise de memory leaks e erros com Valgrind
# Autor: Grupo 15
# Data: 14 de novembro de 2025

set -e

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Diretórios
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BIN_DIR="$PROJECT_ROOT/bin"
OUTPUT_DIR="$PROJECT_ROOT/output/valgrind"
MONITOR_BIN="$BIN_DIR/monitor"

# Criar diretório de output
mkdir -p "$OUTPUT_DIR"

echo -e "${BLUE}=== Análise de Memory Leaks com Valgrind ===${NC}"
echo ""

# Verificar se valgrind está instalado
if ! command -v valgrind &> /dev/null; then
    echo -e "${RED}[ERRO] Valgrind não encontrado!${NC}"
    echo ""
    echo "Para instalar:"
    echo "  Ubuntu/Debian: sudo apt install valgrind"
    echo "  Arch Linux:    sudo pacman -S valgrind"
    echo "  Fedora:        sudo dnf install valgrind"
    echo ""
    exit 1
fi

# Verificar se o binário existe
if [ ! -f "$MONITOR_BIN" ]; then
    echo -e "${RED}[ERRO] Binário não encontrado: $MONITOR_BIN${NC}"
    echo "Execute 'make all' primeiro"
    exit 1
fi

echo -e "${GREEN}[OK] Valgrind encontrado: $(valgrind --version)${NC}"
echo -e "${GREEN}[OK] Binário encontrado: $MONITOR_BIN${NC}"
echo ""

# Função para executar teste com valgrind
run_valgrind_test() {
    local test_name="$1"
    local test_cmd="$2"
    local log_file="$OUTPUT_DIR/${test_name}.log"
    local xml_file="$OUTPUT_DIR/${test_name}.xml"
    
    echo -e "${YELLOW}▶ Teste: $test_name${NC}"
    echo "  Comando: $test_cmd"
    echo "  Log: $log_file"
    
    # Executar valgrind com várias flags
    valgrind \
        --leak-check=full \
        --show-leak-kinds=all \
        --track-origins=yes \
        --verbose \
        --log-file="$log_file" \
        --xml=yes \
        --xml-file="$xml_file" \
        $test_cmd &> /dev/null
    
    # Analisar resultados
    local leaks=$(grep -c "definitely lost" "$log_file" || echo "0")
    local errors=$(grep "ERROR SUMMARY:" "$log_file" | awk '{print $4}')
    
    if [ "$errors" = "0" ] && [ "$leaks" = "0" ]; then
        echo -e "${GREEN}  [OK] Sem leaks ou erros${NC}"
    else
        echo -e "${RED}  [X] Encontrados $errors erros e $leaks leaks${NC}"
    fi
    echo ""
}

# Função para resumir resultados
generate_summary() {
    local summary_file="$OUTPUT_DIR/SUMMARY.md"
    
    echo "# Valgrind Analysis Summary" > "$summary_file"
    echo "" >> "$summary_file"
    echo "**Data:** $(date '+%Y-%m-%d %H:%M:%S')" >> "$summary_file"
    echo "**Valgrind Version:** $(valgrind --version)" >> "$summary_file"
    echo "" >> "$summary_file"
    echo "## Testes Executados" >> "$summary_file"
    echo "" >> "$summary_file"
    
    for log_file in "$OUTPUT_DIR"/*.log; do
        if [ -f "$log_file" ]; then
            local test_name=$(basename "$log_file" .log)
            local errors=$(grep "ERROR SUMMARY:" "$log_file" | awk '{print $4}' || echo "N/A")
            local definitely_lost=$(grep "definitely lost:" "$log_file" | awk '{print $4}' || echo "0")
            local indirectly_lost=$(grep "indirectly lost:" "$log_file" | awk '{print $4}' || echo "0")
            local possibly_lost=$(grep "possibly lost:" "$log_file" | awk '{print $4}' || echo "0")
            local still_reachable=$(grep "still reachable:" "$log_file" | awk '{print $4}' || echo "0")
            
            echo "### $test_name" >> "$summary_file"
            echo "" >> "$summary_file"
            echo "| Métrica | Valor |" >> "$summary_file"
            echo "|---------|-------|" >> "$summary_file"
            echo "| Erros | $errors |" >> "$summary_file"
            echo "| Definitely Lost | $definitely_lost bytes |" >> "$summary_file"
            echo "| Indirectly Lost | $indirectly_lost bytes |" >> "$summary_file"
            echo "| Possibly Lost | $possibly_lost bytes |" >> "$summary_file"
            echo "| Still Reachable | $still_reachable bytes |" >> "$summary_file"
            echo "" >> "$summary_file"
            
            # Adicionar veredicto
            if [ "$errors" = "0" ] && [ "$definitely_lost" = "0" ]; then
                echo "**Status:** [OK] PASSOU (sem leaks ou erros)" >> "$summary_file"
            else
                echo "**Status:** [ERRO] FALHOU (leaks ou erros detectados)" >> "$summary_file"
            fi
            echo "" >> "$summary_file"
        fi
    done
    
    echo -e "${GREEN}[OK] Resumo gerado: $summary_file${NC}"
}

# Menu interativo
show_menu() {
    echo -e "${BLUE}=== Testes Disponíveis ===${NC}"
    echo "1. Teste de Help (sem argumentos)"
    echo "2. Teste de Monitor com PID inválido"
    echo "3. Teste de Namespace list"
    echo "4. Teste de Unit Tests (test_cpu, test_memory, test_io)"
    echo "5. Teste Completo (monitorar processo real por 5s)"
    echo "a. Executar TODOS os testes"
    echo "r. Gerar relatório resumido"
    echo "q. Sair"
    echo ""
}

# Loop principal
while true; do
    show_menu
    read -p "Escolha uma opção: " choice
    echo ""
    
    case $choice in
        1)
            run_valgrind_test "test_help" "$MONITOR_BIN"
            ;;
        2)
            run_valgrind_test "test_invalid_pid" "$MONITOR_BIN monitor 999999 1 2"
            ;;
        3)
            run_valgrind_test "test_namespace_list" "$MONITOR_BIN namespace list 1"
            ;;
        4)
            if [ -f "$BIN_DIR/test_cpu" ]; then
                run_valgrind_test "test_unit_cpu" "$BIN_DIR/test_cpu"
            else
                echo -e "${YELLOW}⚠ test_cpu não compilado. Execute 'make tests' primeiro${NC}"
            fi
            
            if [ -f "$BIN_DIR/test_memory" ]; then
                run_valgrind_test "test_unit_memory" "$BIN_DIR/test_memory"
            else
                echo -e "${YELLOW}⚠ test_memory não compilado. Execute 'make tests' primeiro${NC}"
            fi
            
            if [ -f "$BIN_DIR/test_io" ]; then
                run_valgrind_test "test_unit_io" "$BIN_DIR/test_io"
            else
                echo -e "${YELLOW}⚠ test_io não compilado. Execute 'make tests' primeiro${NC}"
            fi
            ;;
        5)
            echo "Iniciando processo de teste (sleep 10s)..."
            sleep 10 &
            TEST_PID=$!
            echo "PID do processo: $TEST_PID"
            run_valgrind_test "test_monitor_real" "$MONITOR_BIN monitor $TEST_PID 1 5"
            kill $TEST_PID 2>/dev/null || true
            ;;
        a|A)
            echo -e "${BLUE}=== Executando TODOS os testes ===${NC}"
            echo ""
            
            # Teste 1: Help
            run_valgrind_test "test_help" "$MONITOR_BIN"
            
            # Teste 2: PID inválido
            run_valgrind_test "test_invalid_pid" "$MONITOR_BIN monitor 999999 1 2"
            
            # Teste 3: Namespace
            run_valgrind_test "test_namespace_list" "$MONITOR_BIN namespace list 1"
            
            # Teste 4: Unit tests (se compilados)
            if [ -f "$BIN_DIR/test_cpu" ]; then
                run_valgrind_test "test_unit_cpu" "$BIN_DIR/test_cpu"
            fi
            if [ -f "$BIN_DIR/test_memory" ]; then
                run_valgrind_test "test_unit_memory" "$BIN_DIR/test_memory"
            fi
            if [ -f "$BIN_DIR/test_io" ]; then
                run_valgrind_test "test_unit_io" "$BIN_DIR/test_io"
            fi
            
            # Teste 5: Processo real
            echo "Iniciando processo de teste (sleep 10s)..."
            sleep 10 &
            TEST_PID=$!
            echo "PID do processo: $TEST_PID"
            run_valgrind_test "test_monitor_real" "$MONITOR_BIN monitor $TEST_PID 1 5"
            kill $TEST_PID 2>/dev/null || true
            
            echo -e "${GREEN}=== Todos os testes concluídos ===${NC}"
            echo ""
            ;;
        r|R)
            generate_summary
            cat "$OUTPUT_DIR/SUMMARY.md"
            echo ""
            ;;
        q|Q)
            echo "Saindo..."
            break
            ;;
        *)
            echo -e "${RED}Opção inválida!${NC}"
            echo ""
            ;;
    esac
done

echo ""
echo -e "${BLUE}=== Análise Concluída ===${NC}"
echo "Logs salvos em: $OUTPUT_DIR"
echo ""
echo "Para visualizar um log específico:"
echo "  less $OUTPUT_DIR/<test_name>.log"
echo ""
echo "Para gerar relatório resumido:"
echo "  bash $0 (escolha opção 'r')"
echo ""
