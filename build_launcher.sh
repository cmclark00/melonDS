#!/bin/bash

# Build melonDS minimal SDL launcher for handheld
# This script builds the frontend after the core libraries are built

set -e

echo "=== Building melonDS Launcher ==="

# Check if core libraries exist
if [ ! -f "build-handheld/src/libcore.a" ]; then
    echo "Error: Core libraries not found. Run ./build_handheld.sh first"
    exit 1
fi

# Create launcher build directory
mkdir -p build-launcher
cd build-launcher

# Copy the launcher CMakeLists.txt
cp ../CMakeLists_launcher.txt CMakeLists.txt

# Configure the launcher build (current directory, not parent)
cmake .

echo "=== Building launcher ==="
make -j2

echo "=== Launcher build complete! ==="
echo "Binary location: $(pwd)/melonDS"
echo ""
echo "=== Integration with MUOS: ==="
echo "1. Copy melonDS to your handheld device"
echo "2. Replace the DraStic launch script with the new melonDS launcher"
echo "3. The launcher takes a ROM file as argument: ./melonDS /path/to/game.nds" 