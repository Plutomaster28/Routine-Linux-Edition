#!/bin/bash
# Build all extensions in the lib directory

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Routine Bot - Extension Builder ==="
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Count extensions
C_COUNT=$(ls -1 *.c 2>/dev/null | wc -l)
CPP_COUNT=$(ls -1 *.cpp 2>/dev/null | wc -l)
ASM_COUNT=$(ls -1 *.asm *.s 2>/dev/null | wc -l)
TOTAL=$((C_COUNT + CPP_COUNT + ASM_COUNT))

echo "Found:"
echo "  - $C_COUNT C extension(s)"
echo "  - $CPP_COUNT C++ extension(s)"
echo "  - $ASM_COUNT Assembly extension(s)"
echo ""

if [ $TOTAL -eq 0 ]; then
    echo -e "${YELLOW}No extension source files found!${NC}"
    echo "Place .c, .cpp, or .asm/.s files in the lib/ directory"
    exit 0
fi

# Build using CMake
echo "Building extensions using CMake..."
mkdir -p build_extensions
cd build_extensions

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

# Copy built extensions back to lib directory
echo ""
echo "Copying extensions..."

if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
    # Windows
    if ls *.dll 1> /dev/null 2>&1; then
        find . -maxdepth 1 -type f -name '*.dll' \
            ! -name 'fortran_extension.dll' -exec cp -v {} ../ \;
        echo -e "${GREEN}✓ Copied DLL extensions to lib/ directory${NC}"
    else
        echo -e "${YELLOW}⚠ No DLL files found to copy${NC}"
    fi
else
    # Linux/Unix
    if ls *.so 1> /dev/null 2>&1; then
        find . -maxdepth 1 -type f -name '*.so' \
            ! -name 'fortran_extension.so' -exec cp -v {} ../ \;
        echo -e "${GREEN}✓ Copied SO extensions to lib/ directory${NC}"
    else
        echo -e "${YELLOW}⚠ No SO files found to copy${NC}"
    fi
fi

cd ..

echo ""
echo -e "${GREEN}✓ Extension build complete!${NC}"
echo ""

# List built extensions
echo "Built extensions:"
if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
    ls -lh *.dll 2>/dev/null || echo "  (none)"
else
    ls -lh *.so 2>/dev/null || echo "  (none)"
fi

echo ""
echo -e "${GREEN}Extensions are ready to use!${NC}"
echo "Restart the bot to load them."
