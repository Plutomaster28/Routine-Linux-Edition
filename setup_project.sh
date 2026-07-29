#!/bin/bash
# Project Setup Script (without dependency installation)
# Run this to set up project structure and configuration

set -e

echo "=========================================="
echo "Setting Up Discord Bot Project"
echo "=========================================="
echo ""

# Create necessary directories
echo "Creating project directories..."
mkdir -p modules
mkdir -p lib
mkdir -p logs
mkdir -p build
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
        echo "config.example.json not found"
        echo "   Cannot create config.json automatically"
    fi
else
    echo "✓ config.json already exists"
fi
echo ""

# Check for existing build
if [ -d "build" ]; then
    echo "✓ Build directory exists"
    if [ -f "build/CMakeCache.txt" ]; then
        echo "✓ CMake cache found (project already configured)"
    else
        echo "Build directory exists but not configured"
        echo "   Run: cmake -S . -B build -G Ninja"
    fi
else
    echo "Build directory created but not configured"
fi
echo ""

echo "=========================================="
echo "Setup Complete! Next steps:"
echo "=========================================="
echo ""

# Check if already built
if [ -f "build/routine.exe" ] || [ -f "build/routine" ]; then
    echo "✓ Executable found! Ready to run."
    echo ""
    echo "To run the bot:"
    if [ -f "build/routine.exe" ]; then
        echo "   ./build/routine.exe"
    else
        echo "   ./build/routine"
    fi
else
    echo "To build and run:"
    echo "1. Configure CMake (if not done):"
    echo "   cmake -S . -B build -G Ninja"
    echo ""
    echo "2. Build the project:"
    echo "   ninja -C build"
    echo ""
    echo "3. Run the bot:"
    echo "   ./build/routine.exe    # Windows"
    echo "   ./build/routine        # Linux/macOS"
fi

echo ""
echo "See QUICKSTART.md for detailed instructions!"
echo ""
