#!/bin/bash

# melonDS ARM64 Gaming Handheld Build Script
# Optimized for Allwinner H700 SoC with 1GB RAM

set -e

echo "=== melonDS ARM64 Gaming Handheld Build ==="
echo "Target: ARM64 Linux with 1GB RAM"
echo "SoC: Allwinner H700 (max 2GHz)"

# Create build directory
mkdir -p build-handheld
cd build-handheld

# Configure with aggressive optimizations for ARM64 handheld
cmake .. \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DCMAKE_C_FLAGS="-march=armv8-a -mtune=cortex-a73 -O3 -flto -ffast-math -pipe -fomit-frame-pointer -DNDEBUG" \
    -DCMAKE_CXX_FLAGS="-march=armv8-a -mtune=cortex-a73 -O3 -flto -ffast-math -pipe -fomit-frame-pointer -DNDEBUG" \
    -DCMAKE_EXE_LINKER_FLAGS="-flto -Wl,--gc-sections -Wl,--strip-all" \
    -DENABLE_JIT=ON \
    -DENABLE_OGLRENDERER=OFF \
    -DENABLE_LTO=ON \
    -DBUILD_QT_SDL=ON \
    -DUSE_QT6=OFF \
    -G Ninja

echo "=== Building with optimizations for handheld ==="
# Use limited parallel jobs to avoid OOM on 1GB RAM
ninja -j2

echo "=== Build complete! ==="
echo "Binary location: $(pwd)/melonDS"
echo ""
echo "=== Recommended runtime settings for your handheld: ==="
echo "1. Use software renderer (OpenGL disabled)"
echo "2. Limit framerate to 30 FPS or less"
echo "3. Use small audio buffer"
echo "4. Disable JIT if still too slow"
echo "5. Use 1x or 2x screen scaling max"