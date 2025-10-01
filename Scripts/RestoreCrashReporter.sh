#!/bin/bash

# Script to re-enable macOS crash reporter after AFL++ fuzzing

set -e

# Color codes
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_info "Re-enabling macOS crash reporter..."

SL=/System/Library
PL=com.apple.ReportCrash

# Re-enable user-level crash reporter
print_info "Re-enabling user-level crash reporter..."
if launchctl load -w ${SL}/LaunchAgents/${PL}.plist 2>/dev/null; then
    print_success "User-level crash reporter enabled"
else
    print_warning "User-level crash reporter may already be enabled"
fi

# Re-enable system-level crash reporter
print_info "Re-enabling system-level crash reporter (requires sudo)..."
if sudo launchctl load -w ${SL}/LaunchDaemons/${PL}.Root.plist 2>/dev/null; then
    print_success "System-level crash reporter enabled"
else
    print_warning "System-level crash reporter may already be enabled"
fi

print_success "macOS crash reporter has been restored to normal operation"
print_info "Your system will now show crash dialogs again as usual"
