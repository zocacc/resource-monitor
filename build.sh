#!/bin/bash

# ============================================================
# Build Script - Resource Monitor
# Compila o projeto e executa o menu interativo
# ============================================================

set -e

# Cores
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo ""
echo -e "${BLUE}╔═══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║         RESOURCE MONITOR - BUILD & RUN                    ║${NC}"
echo -e "${BLUE}╚═══════════════════════════════════════════════════════════╝${NC}"
echo ""

# Compilar
echo -e "${YELLOW}[1/2] Compilando projeto...${NC}"
make clean > /dev/null 2>&1
if make 2>&1 | grep -q "error:"; then
    echo -e "${RED}✗ Erro na compilação${NC}"
    make
    exit 1
fi
echo -e "${GREEN}✓ Compilação concluída com sucesso${NC}"
echo ""

# Criar diretórios de output
echo -e "${YELLOW}Criando diretórios de output...${NC}"
mkdir -p output/graphs output/experiments
# Garantir permissões corretas para o usuário atual
if [ -w output ]; then
    chown -R $USER:$USER output/ 2>/dev/null || true
fi
echo -e "${GREEN}✓ Diretórios criados${NC}"
echo ""

# Executar menu
echo -e "${YELLOW}[2/2] Iniciando menu interativo...${NC}"
echo ""
echo -e "${BLUE}Dica: Para visualizações de experimentos:${NC}"
echo -e "  • Menu → Opção 4 (Experimentos) → Opção 7 (Gerar visualizações)"
echo -e "  • Ou manual: python3 scripts/visualize.py --experiments output/graphs"
echo ""
./bin/monitor menu
