#!/bin/bash
# Build all modules in the modules directory

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Routine Bot - Module Builder ==="
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Count modules
C_COUNT=$(ls -1 *.c 2>/dev/null | wc -l)
CPP_COUNT=$(ls -1 *.cpp 2>/dev/null | wc -l)
ASM_COUNT=$(ls -1 *.asm *.s 2>/dev/null | wc -l)
TOTAL=$((C_COUNT + CPP_COUNT + ASM_COUNT))

echo "Found:"
echo "  - $C_COUNT C module(s)"
echo "  - $CPP_COUNT C++ module(s)"
echo "  - $ASM_COUNT Assembly module(s)"
echo ""

if [ $TOTAL -eq 0 ]; then
    echo -e "${YELLOW}No module source files found!${NC}"
    echo "Place .c, .cpp, or .asm/.s files in the modules/ directory"
    exit 0
fi

# Build using CMake
echo "Building modules using CMake..."
mkdir -p build_modules
cd build_modules

cmake -G Ninja ..
if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
fi

ninja
if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

# Copy built modules back to modules directory
echo ""
echo "Copying modules..."

if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
    # Windows
    if ls *.dll 1> /dev/null 2>&1; then
        cp -v *.dll ../
        echo -e "${GREEN}✓ Copied DLL modules to modules/ directory${NC}"
    else
        echo -e "${YELLOW}⚠ No DLL files found to copy${NC}"
    fi
else
    # Linux/Unix
    if ls *.so 1> /dev/null 2>&1; then
        cp -v *.so ../
        echo -e "${GREEN}✓ Copied SO modules to modules/ directory${NC}"
    else
        echo -e "${YELLOW}⚠ No SO files found to copy${NC}"
    fi
fi

cd ..

echo ""
echo -e "${GREEN}✓ Module build complete!${NC}"
echo ""

# List built modules
echo "Built modules:"
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
    ls -lh *.dll 2>/dev/null || echo "  (none)"
else
    ls -lh *.so 2>/dev/null || echo "  (none)"
fi

echo ""
echo -e "${GREEN}Modules are ready to use!${NC}"
echo "Restart the bot or use ~reload to load them."
