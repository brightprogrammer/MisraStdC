#!/bin/bash

# Docker-based AFL++ fuzzing script for MisraStdC
# This script builds and runs AFL++ fuzzing with AddressSanitizer (ASAN) in a Docker container
# ASAN is always used for comprehensive memory bug detection

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

print_info "AFL++ fuzzing with AddressSanitizer (ASAN) for MisraStdC"
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

# Detect architecture
ARCH=$(uname -m)
print_info "Detected architecture: $ARCH"

# Build the Docker image with AFL++ and ASAN support
print_info "Building Docker image for AFL++ fuzzing with ASAN on $ARCH..."
print_info "Using Ubuntu 22.04 as base with AFL++ built from source"
if docker build -f Dockerfile.fuzz -t misra-fuzz .; then
    print_success "Docker image built successfully for $ARCH"
else
    print_error "Failed to build Docker image"
    exit 1
fi

# Create output directory on host with proper permissions
mkdir -p "$PROJECT_ROOT/fuzz-outputs"
chmod 755 "$PROJECT_ROOT/fuzz-outputs"

# Clean up any previous fuzzing outputs to avoid conflicts
print_info "Cleaning up previous fuzzing outputs..."
rm -rf "$PROJECT_ROOT/fuzz-outputs"/*

# Run AFL++ fuzzing with ASAN
print_info "Starting AFL++ fuzzing with AddressSanitizer..."
print_info "This will build the project with ASAN and start fuzzing"
print_info "Press Ctrl+C to stop fuzzing"
echo

docker run --rm \
    -v "$PROJECT_ROOT:/src" \
    -v "$PROJECT_ROOT/fuzz-outputs:/src/fuzz/outputs" \
    misra-fuzz \
    bash -c "
        echo 'Building AFL++ fuzzing harness with AddressSanitizer...'
        /usr/local/bin/fuzz.sh
    "

print_success "Fuzzing session completed!"
print_info "Check fuzz-outputs directory for results:"
print_info "  $PROJECT_ROOT/fuzz-outputs"
echo
print_info "AFL++ output structure:"
print_info "  - fuzzer-asan/crashes/  : Unique crash inputs that caused the program to crash"
print_info "  - fuzzer-asan/hangs/    : Inputs that caused the program to hang/timeout"
print_info "  - fuzzer-asan/queue/    : Test cases that found new code paths"
print_info "  - fuzzer-asan/plot_data : Statistics and performance data"
print_info "  - fuzzer-asan/fuzzer_stats : Current fuzzing statistics"
echo
print_info "To analyze crashes:"
print_info "  - Check fuzzer-asan/crashes/ directory for crash inputs"
print_info "  - Use: ./FuzzHarness < crash_file to reproduce crashes"
print_info "  - Check fuzzer-asan/fuzzer_stats for coverage information"
