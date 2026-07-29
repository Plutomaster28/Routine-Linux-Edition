#!/bin/bash
# Linux Dependency Installation Script
# Supports: Ubuntu, Debian, Raspberry Pi OS

set -e

ROUTINE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROUTINE_ROOT"

echo "=========================================="
echo "Installing Discord Bot Dependencies"
echo "Platform: Linux"
echo "=========================================="
echo ""

# Detect distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    DISTRO=$ID
    VERSION=$VERSION_ID
    echo "Detected: $NAME $VERSION"
else
    echo "Cannot detect Linux distribution"
    DISTRO="unknown"
fi

# Detect architecture
ARCH=$(uname -m)
echo "Architecture: $ARCH"
echo ""

if [[ "$ARCH" == "aarch64" ]]; then
    echo "ARM64 detected - will use optimized build flags"
elif [[ "$ARCH" == "armv7l" ]] || [[ "$ARCH" == "armv7"* ]]; then
    echo "ARM32 detected - will use optimized build flags"
elif [[ "$ARCH" == "x86_64" ]]; then
    echo "x86_64 detected"
else
    echo "Unknown architecture: $ARCH"
fi
echo ""

# Check for sudo
if [ "$EUID" -eq 0 ]; then
    SUDO=""
    echo "Running as root"
else
    SUDO="sudo"
    echo "Using sudo for package installation"
fi
echo ""

# Install based on distribution
case "$DISTRO" in
    ubuntu|debian|raspbian)
        echo "Updating package lists..."
        $SUDO apt-get update

        echo ""
        echo "Installing build tools..."
        $SUDO apt-get install -y \
            build-essential \
            cmake \
            ninja-build \
            git \
            pkg-config

        echo ""
        echo "Installing FORTRAN compiler..."
        $SUDO apt-get install -y gfortran

        echo ""
        echo "Installing libraries..."
        $SUDO apt-get install -y \
            libcurl4-openssl-dev \
            libidn2-dev \
            libssl-dev \
            libwebsockets-dev \
            nlohmann-json3-dev \
            libyaml-cpp-dev

        echo ""
        echo "Installing Lua..."
        # Try Lua 5.4 first, fallback to 5.3 or 5.1
        if $SUDO apt-cache show lua5.4 > /dev/null 2>&1; then
            $SUDO apt-get install -y lua5.4 liblua5.4-dev
            echo "✓ Installed Lua 5.4"
        elif $SUDO apt-cache show lua5.3 > /dev/null 2>&1; then
            $SUDO apt-get install -y lua5.3 liblua5.3-dev
            echo "✓ Installed Lua 5.3"
        else
            $SUDO apt-get install -y lua5.1 liblua5.1-dev
            echo "✓ Installed Lua 5.1"
        fi
        ;;

    fedora|rhel|centos)
        echo "Updating package lists..."
        $SUDO dnf check-update || true

        echo ""
        echo "Installing build tools..."
        $SUDO dnf install -y \
            gcc \
            gcc-c++ \
            cmake \
            ninja-build \
            git \
            pkgconfig

        echo ""
        echo "Installing FORTRAN compiler..."
        $SUDO dnf install -y gcc-gfortran

        echo ""
        echo "Installing libraries..."
        $SUDO dnf install -y \
            libcurl-devel \
            openssl-devel \
            libwebsockets-devel \
            json-devel \
            yaml-cpp-devel \
            lua-devel
        ;;

    arch|manjaro)
        echo "Updating package lists..."
        $SUDO pacman -Sy

        echo ""
        echo "Installing build tools..."
        $SUDO pacman -S --noconfirm --needed \
            base-devel \
            cmake \
            ninja \
            git

        echo ""
        echo "Installing FORTRAN compiler..."
        $SUDO pacman -S --noconfirm --needed gcc-fortran

        echo ""
        echo "Installing libraries..."
        $SUDO pacman -S --noconfirm --needed \
            curl \
            openssl \
            libwebsockets \
            nlohmann-json \
            yaml-cpp \
            lua
        ;;

    *)
        echo "Unsupported distribution: $DISTRO"
        echo ""
        echo "Please install these packages manually:"
        echo "  - gcc, g++, cmake, ninja-build, gfortran"
        echo "  - libcurl-dev, libssl-dev, libwebsockets-dev"
        echo "  - nlohmann-json3-dev, libyaml-cpp-dev, lua-dev"
        exit 1
        ;;
esac

echo ""
echo "=========================================="
echo "All dependencies installed successfully!"
echo "=========================================="
echo ""

# Create necessary directories
echo "Creating project directories..."
mkdir -p modules
mkdir -p lib
mkdir -p logs
echo "✓ Directories created"
echo ""

# Setup config.json if it doesn't exist
if [ ! -f "config.json" ]; then
    if [ -f "config.example.json" ]; then
        echo "Creating config.json from template..."
        cp config.example.json config.json
        echo "✓ config.json created"
        echo ""
        echo "IMPORTANT: Edit config.json and add your Discord bot token!"
        echo "   Get your token from: https://discord.com/developers/applications"
    else
        echo "config.example.json not found, skipping config.json creation"
    fi
else
    echo "✓ config.json already exists"
fi
echo ""

echo "=========================================="
echo "Setup Complete!"
echo "=========================================="
echo "System Info:"
echo "  Distribution: $DISTRO"
echo "  Architecture: $ARCH"
echo ""
echo "Building the complete Linux distribution..."
./build_linux.sh
echo ""
echo "Next steps:"
echo "1. Edit config.json and add your Discord bot token"
echo "2. Run: ./build-linux/kernel/routine"
echo ""

# Check for special ARM notes
if [[ "$ARCH" == "aarch64" ]] || [[ "$ARCH" == "armv7"* ]]; then
    echo "ARM Note:"
    echo "  CMake will automatically apply ARM-specific optimizations"
    echo "  ARM64: -march=armv8-a"
    echo "  ARM32: -march=armv7-a -mfpu=neon"
    echo ""
fi

echo "See QUICKSTART.md for detailed instructions!"
echo ""
