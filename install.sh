#!/bin/bash
#
# install.sh - Automated installer for edit text editor
# Detects OS, installs dependencies, and builds/installs the editor
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Print functions
print_header() {
    echo ""
    echo -e "${CYAN}${BOLD}"
    echo "  ╔═══════════════════════════════════════╗"
    echo "  ║                                       ║"
    echo "  ║     edit v2.0.0 - Text Editor         ║"
    echo "  ║     by @rahuldangeofficial            ║"
    echo "  ║                                       ║"
    echo "  ╚═══════════════════════════════════════╝"
    echo -e "${NC}"
}

print_step() {
    echo -e "${BLUE}[*]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

# Detect OS
detect_os() {
    OS="$(uname -s)"
    print_step "Detecting operating system..."
    
    case "$OS" in
        Linux)
            if [ -f /etc/debian_version ]; then
                DISTRO="debian"
                print_success "Detected: Debian/Ubuntu"
            elif [ -f /etc/redhat-release ]; then
                DISTRO="rhel"
                print_success "Detected: RHEL/Fedora/CentOS"
            elif [ -f /etc/arch-release ]; then
                DISTRO="arch"
                print_success "Detected: Arch Linux"
            elif [ -f /etc/alpine-release ]; then
                DISTRO="alpine"
                print_success "Detected: Alpine Linux"
            else
                DISTRO="unknown"
                print_warning "Unknown Linux distribution"
            fi
            ;;
        Darwin)
            DISTRO="macos"
            print_success "Detected: macOS"
            ;;
        *)
            print_error "Unsupported OS: $OS"
            exit 1
            ;;
    esac
}

# Install dependencies
install_deps() {
    print_step "Checking dependencies..."
    
    case "$DISTRO" in
        debian)
            print_step "Installing build tools and ncurses..."
            sudo apt-get update -qq
            sudo apt-get install -y -qq build-essential libncursesw5-dev > /dev/null
            print_success "Dependencies installed"
            ;;
        rhel)
            print_step "Installing build tools and ncurses..."
            sudo dnf install -y -q gcc-c++ ncurses-devel make 2>/dev/null || \
            sudo yum install -y -q gcc-c++ ncurses-devel make 2>/dev/null
            print_success "Dependencies installed"
            ;;
        arch)
            print_step "Installing build tools and ncurses..."
            sudo pacman -Sy --noconfirm --quiet base-devel ncurses > /dev/null
            print_success "Dependencies installed"
            ;;
        alpine)
            print_step "Installing build tools and ncurses..."
            sudo apk add --no-cache --quiet build-base ncurses-dev > /dev/null
            print_success "Dependencies installed"
            ;;
        macos)
            if ! xcode-select -p &> /dev/null; then
                print_step "Installing Xcode Command Line Tools..."
                xcode-select --install
                print_warning "Please re-run this script after Xcode tools are installed."
                exit 0
            fi
            print_success "Build tools available"
            ;;
        *)
            print_warning "Please install manually: g++, make, ncurses-devel"
            ;;
    esac
}

# Build
build() {
    print_step "Building edit..."
    make clean > /dev/null 2>&1 || true
    
    if make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1) > /dev/null 2>&1; then
        print_success "Build complete"
    else
        print_error "Build failed"
        exit 1
    fi
}

# Install
install() {
    print_step "Installing to /usr/local/bin (requires sudo)..."
    
    if sudo make install > /dev/null 2>&1; then
        print_success "Installation complete"
    else
        print_error "Installation failed"
        exit 1
    fi
}

# Main
print_header
detect_os
install_deps
build
install

echo ""
echo -e "${GREEN}${BOLD}  ══════════════════════════════════════════${NC}"
echo -e "${GREEN}${BOLD}    Installation successful!${NC}"
echo -e "${GREEN}${BOLD}    Run: ${CYAN}edit <filename>${GREEN}${BOLD} to start${NC}"
echo -e "${GREEN}${BOLD}  ══════════════════════════════════════════${NC}"
echo ""
