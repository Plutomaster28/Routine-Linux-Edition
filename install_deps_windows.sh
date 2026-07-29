#!/bin/bash
# Windows (MSYS2 UCRT64) Dependency Installation Script
# Run this in MSYS2 UCRT64 terminal

set -e

echo "=========================================="
echo "Installing Discord Bot Dependencies"
echo "Platform: Windows (MSYS2 UCRT64)"
echo "=========================================="
echo ""

# Check if running in MSYS2
if [[ ! "$MSYSTEM" == "UCRT64" ]]; then
    echo "WARNING: Not running in MSYS2 UCRT64 environment!"
    echo "Please open 'MSYS2 UCRT64' terminal and run this script again."
    echo ""
    echo "If MSYS2 is not installed:"
    echo "1. Download from: https://www.msys2.org/"
    echo "2. Install and open 'MSYS2 UCRT64' terminal"
    echo "3. Run this script"
    exit 1
fi

echo "✓ MSYS2 UCRT64 environment detected"
echo ""

# Update package database
echo "Updating package database..."
pacman -Sy --noconfirm

# Install build tools
echo ""
echo "Installing build tools..."
pacman -S --noconfirm --needed \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-gdb \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-ninja \
    mingw-w64-ucrt-x86_64-make

# Install FORTRAN compiler
echo ""
echo "Installing FORTRAN compiler..."
pacman -S --noconfirm --needed \
    mingw-w64-ucrt-x86_64-gcc-fortran

# Install required libraries
echo ""
echo "Installing libraries..."
pacman -S --noconfirm --needed \
    mingw-w64-ucrt-x86_64-curl \
    mingw-w64-ucrt-x86_64-openssl \
    mingw-w64-ucrt-x86_64-libwebsockets \
    mingw-w64-ucrt-x86_64-nlohmann-json \
    mingw-w64-ucrt-x86_64-lua \
    mingw-w64-ucrt-x86_64-yaml-cpp

# Install git (useful for version control)
echo ""
echo "Installing git..."
pacman -S --noconfirm --needed git

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
echo "Setup Complete! Next steps:"
echo "=========================================="
echo "1. Edit config.json and add your Discord bot token"
echo "2. Build the project:"
echo "   cmake -S . -B build -G Ninja"
echo "   ninja -C build"
echo "3. Run: ./build/routine.exe"
echo ""
echo "See QUICKSTART.md for detailed instructions!"
echo ""
