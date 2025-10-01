#!/bin/bash

# Docker-based fuzzing script for MisraStdC
# This script builds and runs AFL++ fuzzing in a Docker container

set -e

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

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

print_info "Docker-based AFL++ fuzzing setup for MisraStdC"
print_info "Project root: $PROJECT_ROOT"

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    print_error "Docker is not installed or not in PATH"
    print_info "Please install Docker: https://docs.docker.com/get-docker/"
    exit 1
fi

# Check if Docker is running
if ! docker info &> /dev/null; then
    print_error "Docker is not running"
    print_info "Please start Docker and try again"
    exit 1
fi

print_success "Docker is available and running"

# Detect architecture and build accordingly
ARCH=$(uname -m)
print_info "Detected architecture: $ARCH"

# Build the Docker image using Ubuntu base with AFL++ from source
print_info "Building Docker image for AFL++ fuzzing on $ARCH..."
print_info "Using Ubuntu 22.04 as base with AFL++ built from source (supports all architectures)"
if docker build -f Dockerfile.fuzz -t misra-fuzz .; then
    print_success "Docker image built successfully for $ARCH"
else
    print_error "Failed to build Docker image"
    exit 1
fi

# Set fuzzing mode to ASAN only (for CI)
FUZZ_MODE="asan"
print_info "Running AFL++ with AddressSanitizer (ASAN) for better bug detection"

# Run the container
print_info "Starting AFL++ fuzzing with $FUZZ_MODE mode..."
print_info "Press Ctrl+C to stop fuzzing"
echo

# Create output directory on host with proper permissions
mkdir -p "$PROJECT_ROOT/fuzz-outputs"
chmod 755 "$PROJECT_ROOT/fuzz-outputs"

docker run --rm \
    -v "$PROJECT_ROOT:/src" \
    -v "$PROJECT_ROOT/fuzz-outputs:/src/fuzz/outputs" \
    misra-fuzz \
    bash -c "
        echo 'Building AFL++ fuzzing harness with ASAN...'
        ./build_afl_asan.sh
        echo 'Starting fuzzing...'
        ./fuzz.sh $FUZZ_MODE
    "

print_success "Fuzzing session completed!"
print_info "Check fuzz-outputs directory for results:"
print_info "  $PROJECT_ROOT/fuzz-outputs"
echo
print_info "AFL++ output structure:"
print_info "  - crashes/     : Unique crash inputs that caused the program to crash"
print_info "  - hangs/       : Inputs that caused the program to hang/timeout"
print_info "  - queue/       : Test cases that found new code paths"
print_info "  - plot_data    : Statistics and performance data"
print_info "  - fuzzer_stats : Current fuzzing statistics"
echo
print_info "To analyze crashes:"
print_info "  - Check crashes/ directory for crash inputs"
print_info "  - Use: ./FuzzHarness < crash_file to reproduce crashes"
print_info "  - Check fuzzer_stats for coverage information"
