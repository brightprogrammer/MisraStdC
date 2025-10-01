#!/bin/bash

# Fuzzing setup script for macOS
# This script sets up AFL++ fuzzing environment for the MisraStdC library

set -e  # Exit on any error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print functions
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    print_error "This script is designed for macOS only"
    exit 1
fi

print_info "Setting up AFL++ fuzzing environment for MisraStdC"

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

print_info "Project root: $PROJECT_ROOT"

# Check for Homebrew
if ! command -v brew &> /dev/null; then
    print_error "Homebrew is required but not installed."
    print_info "Install Homebrew from https://brew.sh/"
    exit 1
fi

print_success "Homebrew is installed"

# Check and install AFL++
if ! command -v afl-fuzz &> /dev/null; then
    print_info "AFL++ not found. Installing via Homebrew..."
    if brew install afl++; then
        print_success "AFL++ installed successfully"
    else
        print_error "Failed to install AFL++. Please install manually:"
        print_info "  brew install afl++"
        print_info "Or visit: https://github.com/AFLplusplus/AFLplusplus"
        exit 1
    fi
else
    print_success "AFL++ is already installed"
fi

# Check AFL++ version
AFL_VERSION=$(afl-fuzz 2>&1 | head -n 1 || true)
print_info "AFL++ version: $AFL_VERSION"

# Check AFL++ compilers
if ! command -v afl-clang-fast &> /dev/null && ! command -v afl-cc &> /dev/null; then
    print_error "AFL++ compilers not found (afl-clang-fast or afl-cc)"
    print_info "Please ensure AFL++ is properly installed"
    exit 1
fi

# Determine which AFL++ compiler to use
AFL_CC=""
if command -v afl-clang-fast &> /dev/null; then
    AFL_CC="afl-clang-fast"
    AFL_CXX="afl-clang-fast++"
    print_success "Using afl-clang-fast for compilation"
elif command -v afl-cc &> /dev/null; then
    AFL_CC="afl-cc"
    AFL_CXX="afl-c++"
    print_success "Using afl-cc for compilation"
fi

# Find build directory (prefer lowercase 'build')
BUILD_DIR=""
if [ -d "$PROJECT_ROOT/build" ]; then
    BUILD_DIR="$PROJECT_ROOT/build"
elif [ -d "$PROJECT_ROOT/Build" ]; then
    BUILD_DIR="$PROJECT_ROOT/Build"
else
    print_error "No build directory found. Please run 'meson compile -C build' first."
    exit 1
fi

print_success "Using build directory: $BUILD_DIR"

# Create AFL++ instrumented build
AFL_BUILD_DIR="$PROJECT_ROOT/build-afl"
print_info "Setting up AFL++ instrumented build..."

# Clean up old AFL build if it exists
if [ -d "$AFL_BUILD_DIR" ]; then
    print_info "Removing existing AFL++ build directory..."
    rm -rf "$AFL_BUILD_DIR"
fi

# Ask user for fuzzing approach
echo
print_info "Choose fuzzing approach:"
echo "  1) AFL++ fuzzing (coverage-guided, no AddressSanitizer)"
echo "  2) Regular fuzzing with AddressSanitizer (better bug detection)"
echo "  3) Both (create two separate builds)"
echo
read -p "Enter choice (1-3): " -n 1 -r
echo

case $REPLY in
    1)
        FUZZ_APPROACH="afl"
        ;;
    2)
        FUZZ_APPROACH="asan"
        ;;
    3)
        FUZZ_APPROACH="both"
        ;;
    *)
        print_warning "Invalid choice, defaulting to AFL++ fuzzing"
        FUZZ_APPROACH="afl"
        ;;
esac

# Set up AFL++ build environment
export CC="$AFL_CC"
export CXX="$AFL_CXX"
export AFL_HARDEN=1  # Enable hardening features

if [[ "$FUZZ_APPROACH" == "afl" || "$FUZZ_APPROACH" == "both" ]]; then
    print_info "Configuring project with AFL++ compilers..."
    print_warning "Note: AFL++ compilers don't support AddressSanitizer, using debug build instead"
    if ! meson setup "$AFL_BUILD_DIR" --buildtype=debug; then
        print_error "Failed to configure project with AFL++ compilers"
        exit 1
    fi
fi

if [[ "$FUZZ_APPROACH" == "asan" || "$FUZZ_APPROACH" == "both" ]]; then
    # Create ASAN build directory
    ASAN_BUILD_DIR="$PROJECT_ROOT/build-asan"
    print_info "Configuring project with AddressSanitizer..."
    
    # Clean up old ASAN build if it exists
    if [ -d "$ASAN_BUILD_DIR" ]; then
        print_info "Removing existing ASAN build directory..."
        rm -rf "$ASAN_BUILD_DIR"
    fi
    
    # Reset compiler environment for ASAN build
    unset CC
    unset CXX
    unset AFL_HARDEN
    
    if ! meson setup "$ASAN_BUILD_DIR" --buildtype=debug -Db_sanitize=address; then
        print_error "Failed to configure project with AddressSanitizer"
        exit 1
    fi
fi

# Compile based on chosen approach
if [[ "$FUZZ_APPROACH" == "afl" || "$FUZZ_APPROACH" == "both" ]]; then
    print_info "Compiling with AFL++ instrumentation..."
    if ! meson compile -C "$AFL_BUILD_DIR"; then
        print_error "Failed to compile project with AFL++ instrumentation"
        exit 1
    fi
    print_success "AFL++ instrumented build completed"
fi

if [[ "$FUZZ_APPROACH" == "asan" || "$FUZZ_APPROACH" == "both" ]]; then
    print_info "Compiling with AddressSanitizer..."
    if ! meson compile -C "$ASAN_BUILD_DIR"; then
        print_error "Failed to compile project with AddressSanitizer"
        exit 1
    fi
    print_success "AddressSanitizer build completed"
fi

# Set primary build directory based on approach
if [[ "$FUZZ_APPROACH" == "afl" ]]; then
    BUILD_DIR="$AFL_BUILD_DIR"
elif [[ "$FUZZ_APPROACH" == "asan" ]]; then
    BUILD_DIR="$ASAN_BUILD_DIR"
else
    # For "both", use AFL++ as primary for fuzzing commands
    BUILD_DIR="$AFL_BUILD_DIR"
fi

# Check if FuzzHarness executable exists
HARNESS_EXEC="$BUILD_DIR/FuzzHarness"
if [ ! -f "$HARNESS_EXEC" ]; then
    print_error "FuzzHarness executable not found at: $HARNESS_EXEC"
    print_info "Build may have failed"
    exit 1
fi

print_success "AFL++ instrumented FuzzHarness executable found"

# Create fuzzing directories
FUZZ_DIR="$BUILD_DIR/fuzz"
INPUT_DIR="$FUZZ_DIR/inputs"
OUTPUT_DIR="$FUZZ_DIR/outputs"

print_info "Creating fuzzing directories..."
mkdir -p "$INPUT_DIR"
mkdir -p "$OUTPUT_DIR"

# Create seed input files for better fuzzing
print_info "Creating seed input files..."

# Seed 1: Basic VecPushBack operation
# Format: [obj_selector:2][func_selector:2][value:4]
# OBJ_INT_VEC=0, VEC_PUSH_BACK=0, value=0x12345678
echo -ne '\x00\x00\x00\x00\x12\x34\x56\x78' > "$INPUT_DIR/seed1_pushback"

# Seed 2: VecInsert operation  
# Format: [obj_selector:2][func_selector:2][index:2][value:4]
# OBJ_INT_VEC=0, VEC_INSERT=4, index=0, value=0xDEADBEEF
echo -ne '\x00\x00\x00\x04\x00\x00\xDE\xAD\xBE\xEF' > "$INPUT_DIR/seed2_insert"

# Seed 3: VecAt operation
# Format: [obj_selector:2][func_selector:2][index:2]
# OBJ_INT_VEC=0, VEC_AT=7, index=0
echo -ne '\x00\x00\x00\x07\x00\x00' > "$INPUT_DIR/seed3_at"

# Seed 4: VecLen operation
# Format: [obj_selector:2][func_selector:2]
# OBJ_INT_VEC=0, VEC_LEN=8
echo -ne '\x00\x00\x00\x08' > "$INPUT_DIR/seed4_len"

# Seed 5: Sequence of operations
# PushBack + PushFront + PopBack + Len
echo -ne '\x00\x00\x00\x00\x11\x11\x11\x11\x00\x00\x00\x01\x22\x22\x22\x22\x00\x00\x00\x02\x00\x00\x00\x08' > "$INPUT_DIR/seed5_sequence"

# Seed 6: Edge case - empty operations
echo -ne '\x00\x00\x00\x02' > "$INPUT_DIR/seed6_empty"

# Seed 7: VecClear operation 
# Format: [obj_selector:2][func_selector:2]
# OBJ_INT_VEC=0, VEC_CLEAR=12
echo -ne '\x00\x00\x00\x0C' > "$INPUT_DIR/seed7_clear"

# Seed 8: VecResize operation
# Format: [obj_selector:2][func_selector:2][new_size:2]
# OBJ_INT_VEC=0, VEC_RESIZE=13, new_size=10
echo -ne '\x00\x00\x00\x0D\x00\x0A' > "$INPUT_DIR/seed8_resize"

# Seed 9: VecReverse operation
# Format: [obj_selector:2][func_selector:2]
# OBJ_INT_VEC=0, VEC_REVERSE=17
echo -ne '\x00\x00\x00\x11' > "$INPUT_DIR/seed9_reverse"

# Seed 10: VecSwapItems operation
# Format: [obj_selector:2][func_selector:2][idx1:2][idx2:2]
# OBJ_INT_VEC=0, VEC_SWAP_ITEMS=18, idx1=0, idx2=1
echo -ne '\x00\x00\x00\x12\x00\x00\x00\x01' > "$INPUT_DIR/seed10_swap"

# Seed 11: VecInsertRange operation
# Format: [obj_selector:2][func_selector:2][idx:2][count:1][values...]
# OBJ_INT_VEC=0, VEC_INSERT_RANGE=19, idx=0, count=3, values=[1,2,3]
echo -ne '\x00\x00\x00\x13\x00\x00\x03\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x03' > "$INPUT_DIR/seed11_range"

# Seed 12: VecPushBackArray operation
# Format: [obj_selector:2][func_selector:2][count:1][values...]
# OBJ_INT_VEC=0, VEC_PUSH_BACK_ARRAY=26, count=2, values=[100,200]
echo -ne '\x00\x00\x00\x1A\x02\x00\x00\x00\x64\x00\x00\x00\xC8' > "$INPUT_DIR/seed12_array"

# Seed 13: VecSort operation
# Format: [obj_selector:2][func_selector:2]
# OBJ_INT_VEC=0, VEC_SORT=29
echo -ne '\x00\x00\x00\x1D' > "$INPUT_DIR/seed13_sort"

# Seed 14: VecMerge operation
# Format: [obj_selector:2][func_selector:2][count:1][values...]
# OBJ_INT_VEC=0, VEC_MERGE=33, count=2, values=[99,88]
echo -ne '\x00\x00\x00\x21\x02\x00\x00\x00\x63\x00\x00\x00\x58' > "$INPUT_DIR/seed14_merge"

# Seed 15: VecPtrAt operation
# Format: [obj_selector:2][func_selector:2][idx:2]
# OBJ_INT_VEC=0, VEC_PTR_AT=32, idx=0
echo -ne '\x00\x00\x00\x20\x00\x00' > "$INPUT_DIR/seed15_ptr_at"

# Seed 16: VecBegin/End operations
# Format: [obj_selector:2][func_selector:2]
# OBJ_INT_VEC=0, VEC_BEGIN=30
echo -ne '\x00\x00\x00\x1E' > "$INPUT_DIR/seed16_begin"

# Seed 17: Complex sequence with new functions
# PushBack + Sort + Merge + Reverse + SwapItems
echo -ne '\x00\x00\x00\x00\x00\x00\x00\x05\x00\x00\x00\x00\x00\x00\x00\x03\x00\x00\x00\x1D\x00\x00\x00\x21\x01\x00\x00\x00\x42\x00\x00\x00\x11\x00\x00\x00\x12\x00\x00\x00\x01' > "$INPUT_DIR/seed17_complex"

print_success "Created $(ls "$INPUT_DIR" | wc -l | tr -d ' ') seed input files"

# Check system limits for AFL
print_info "Checking system configuration for AFL..."

# Check and handle macOS crash reporter
print_info "Checking macOS crash reporter configuration..."
if launchctl list | grep -q com.apple.ReportCrash; then
    print_warning "macOS crash reporter is enabled and will interfere with AFL++"
    print_info "AFL++ requires disabling crash reporter for proper fuzzing"
    echo
    print_info "This will temporarily disable crash reporting (fully reversible)"
    read -p "Disable crash reporter for fuzzing? (y/N): " -n 1 -r
    echo
    
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        print_info "Disabling macOS crash reporter..."
        
        # Disable user-level crash reporter
        SL=/System/Library
        PL=com.apple.ReportCrash
        
        if launchctl unload -w ${SL}/LaunchAgents/${PL}.plist 2>/dev/null; then
            print_success "Disabled user-level crash reporter"
        else
            print_warning "User-level crash reporter may already be disabled"
        fi
        
        # Disable system-level crash reporter (requires sudo)
        print_info "Disabling system-level crash reporter (requires sudo)..."
        if sudo launchctl unload -w ${SL}/LaunchDaemons/${PL}.Root.plist 2>/dev/null; then
            print_success "Disabled system-level crash reporter"
        else
            print_warning "System-level crash reporter may already be disabled"
        fi
        
        print_success "Crash reporter disabled for AFL++ fuzzing"
        print_info "To re-enable later, run:"
        echo "  launchctl load -w ${SL}/LaunchAgents/${PL}.plist"
        echo "  sudo launchctl load -w ${SL}/LaunchDaemons/${PL}.Root.plist"
        
    else
        print_warning "Crash reporter still enabled - AFL++ may show warnings"
        print_info "You can disable it manually later if needed"
    fi
else
    print_success "macOS crash reporter is already disabled"
fi

# Check core dump pattern (important for AFL)
CORE_PATTERN=$(sysctl -n kern.corefile)
if [[ "$CORE_PATTERN" == *"|"* ]]; then
    print_warning "Core dump pattern contains pipes, which may interfere with AFL"
    print_info "Current pattern: $CORE_PATTERN"
    print_info "Consider running: sudo sysctl -w kern.corefile=core"
fi

# macOS-specific: Check if we need to set CPU affinity
NPROC=$(sysctl -n hw.ncpu)
print_info "Detected $NPROC CPU cores"

# Create a simple test to ensure the harness works
print_info "Testing harness with seed input..."
if "$HARNESS_EXEC" "$INPUT_DIR/seed1_pushback" > /dev/null 2>&1; then
    print_success "Harness test passed"
else
    print_error "Harness test failed. There may be an issue with the executable."
    exit 1
fi

# Display fuzzing information
print_info "=== Fuzzing Environment Setup Complete ==="
echo
print_info "Directories:"
echo "  Input dir:  $INPUT_DIR"
echo "  Output dir: $OUTPUT_DIR"
echo "  Executable: $HARNESS_EXEC"
echo
print_info "Seed files created:"
ls -la "$INPUT_DIR"
echo

# Provide fuzzing commands
print_info "=== Ready to start fuzzing! ==="
echo

if [[ "$FUZZ_APPROACH" == "afl" || "$FUZZ_APPROACH" == "both" ]]; then
    print_success "AFL++ Fuzzing Commands:"
    echo "  cd $AFL_BUILD_DIR"
    echo "  afl-fuzz -i fuzz/inputs -o fuzz/outputs ./FuzzHarness"
    echo
    print_info "AFL++ Alternative options:"
    echo "  # With specific timeout (5 seconds)"
    echo "  afl-fuzz -t 5000 -i fuzz/inputs -o fuzz/outputs ./FuzzHarness"
    echo
    echo "  # With multiple parallel fuzzers (utilize all CPU cores)"
    echo "  afl-fuzz -M fuzzer1 -i fuzz/inputs -o fuzz/outputs ./FuzzHarness &"
    echo "  afl-fuzz -S fuzzer2 -i fuzz/inputs -o fuzz/outputs ./FuzzHarness &"
    echo "  afl-fuzz -S fuzzer3 -i fuzz/inputs -o fuzz/outputs ./FuzzHarness &"
    echo "  # ... add more -S instances for each CPU core"
    echo
    echo "  # Monitor fuzzing progress"
    echo "  afl-whatsup fuzz/outputs"
    echo
    print_success "✅ AFL++ binary compiled for coverage-guided fuzzing!"
    print_info "This enables AFL++ to track code paths and find deeper bugs more efficiently."
    echo
fi

if [[ "$FUZZ_APPROACH" == "asan" || "$FUZZ_APPROACH" == "both" ]]; then
    print_success "AddressSanitizer Fuzzing Commands:"
    echo "  cd $ASAN_BUILD_DIR"
    echo "  # Manual fuzzing with ASAN (better bug detection)"
    echo "  for i in {1..1000}; do"
    echo "    head -c \$((RANDOM % 1000 + 100)) /dev/urandom | ./FuzzHarness"
    echo "  done"
    echo
    echo "  # Or use a fuzzer like honggfuzz or libFuzzer"
    echo "  # honggfuzz -i fuzz/inputs -o fuzz/outputs -- ./FuzzHarness"
    echo
    print_success "✅ AddressSanitizer binary compiled for better bug detection!"
    print_info "This will catch memory errors like use-after-free, buffer overflows, etc."
    echo
fi

print_warning "Note: AFL may show warnings about system configuration on macOS."
print_warning "These are usually non-critical for local fuzzing."
echo

# Ask user if they want to start fuzzing now
read -p "Do you want to start fuzzing now? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    if [[ "$FUZZ_APPROACH" == "afl" || "$FUZZ_APPROACH" == "both" ]]; then
        print_info "Starting AFL++ fuzzing..."
        cd "$AFL_BUILD_DIR"
        exec afl-fuzz -i fuzz/inputs -o fuzz/outputs ./FuzzHarness
    elif [[ "$FUZZ_APPROACH" == "asan" ]]; then
        print_info "Starting AddressSanitizer fuzzing..."
        cd "$ASAN_BUILD_DIR"
        print_info "Running 1000 random test cases with AddressSanitizer..."
        for i in {1..1000}; do
            head -c $((RANDOM % 1000 + 100)) /dev/urandom | ./FuzzHarness
        done
        print_success "AddressSanitizer fuzzing completed!"
    fi
else
    print_info "Setup complete. You can start fuzzing manually using the commands above."
fi
