#!/usr/bin/env bash

# Script to check code formatting with clang-format
# This script mimics the GitHub Actions workflow for local testing

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "🔍 Checking code formatting with clang-format..."
echo ""

# Check if clang-format is available
if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}❌ clang-format is not installed${NC}"
    echo "Please install clang-format:"
    echo "  - Ubuntu/Debian: sudo apt-get install clang-format"
    echo "  - macOS: brew install clang-format"
    echo "  - Windows: Download from LLVM releases"
    exit 1
fi

echo "📋 clang-format version: $(clang-format --version)"
echo ""

# Find all C and header files, excluding build directories and submodules
FILES=$(find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) \
        ! -path "./build*" \
        ! -path "./builddir*" \
        ! -path "./.git/*" \
        ! -path "./demangler/*" \
        ! -path "./.cache/*" \
        -print)

if [ -z "$FILES" ]; then
    echo -e "${YELLOW}⚠️  No C/C++ files found to check${NC}"
    exit 0
fi

echo "📁 Found files to check:"
echo "$FILES" | sed 's/^/  /'
echo ""
# Create a temporary directory for formatted files
TEMP_DIR=$(mktemp -d)
trap "rm -rf $TEMP_DIR" EXIT

EXIT_CODE=0
TOTAL_FILES=0
FORMATTED_FILES=0
MALFORMED_FILES=0

# Check each file
for file in $FILES; do
    if [ -f "$file" ]; then
        TOTAL_FILES=$((TOTAL_FILES + 1))
        echo -n "Checking $(basename "$file")... "
        
        # Format the file and save to temp directory
        TEMP_FILE="$TEMP_DIR/${file#./}"
        mkdir -p "$(dirname "$TEMP_FILE")"
        clang-format "$file" > "$TEMP_FILE"
        
        # Compare original with formatted version
        DIFF_OUTPUT=$(diff -u "$file" "$TEMP_FILE")
        if [ -n "$DIFF_OUTPUT" ]; then
            echo -e "${RED}❌ NOT FORMATTED${NC}"
            echo "  Differences found in: $file"
            
            # Show the diff
            echo "  Expected changes:"
            echo "$DIFF_OUTPUT" | head -20 | sed 's/^/    /'
            if [ "$(echo "$DIFF_OUTPUT" | wc -l)" -gt 20 ]; then
                echo "    ... (output truncated, use 'clang-format $file' to see full diff)"
            fi
            echo ""
            
            MALFORMED_FILES=$((MALFORMED_FILES + 1))
            EXIT_CODE=1
        else
            echo -e "${GREEN}✅ OK${NC}"
            FORMATTED_FILES=$((FORMATTED_FILES + 1))
        fi
    fi
done

echo ""
echo "📊 Summary:"
echo "  Total files checked: $TOTAL_FILES"
echo -e "  Properly formatted: ${GREEN}$FORMATTED_FILES${NC}"
echo -e "  Need formatting: ${RED}$MALFORMED_FILES${NC}"
echo ""

if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}🎉 All files are properly formatted!${NC}"
else
    echo -e "${RED}💥 Some files are not properly formatted.${NC}"
    echo ""
    echo "To fix formatting issues:"
    echo "  • Fix specific file: clang-format -i <file>"
    echo "  • Fix all files: find . -name '*.c' -o -name '*.h' | grep -v build | xargs clang-format -i"
    echo ""
fi

exit $EXIT_CODE 
