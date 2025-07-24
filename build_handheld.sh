#!/bin/bash

# melonDS ARM64 Gaming Handheld Build Script
# Optimized for Allwinner H700 SoC with 1GB RAM

set -e

echo "=== melonDS ARM64 Gaming Handheld Build ==="
echo "Target: ARM64 Linux with 1GB RAM"
echo "SoC: Allwinner H700 (max 2GHz)"

# Cross-compilation settings
CROSS_PATH="/home/corey/x-tools/aarch64-buildroot-linux-gnu"
CROSS_PREFIX="aarch64-buildroot-linux-gnu"

# Check if cross-compiler exists
if [ ! -f "${CROSS_PATH}/bin/${CROSS_PREFIX}-gcc" ]; then
    echo "Error: Cross-compiler not found at ${CROSS_PATH}/bin/${CROSS_PREFIX}-gcc"
    echo "Please ensure your cross-compilation toolchain is installed correctly."
    exit 1
fi

echo "Using cross-compiler: ${CROSS_PATH}/bin/${CROSS_PREFIX}-gcc"

# Create build directory
mkdir -p build-handheld
cd build-handheld

# Configure with aggressive optimizations for ARM64 handheld
cmake .. \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
    -DCMAKE_C_COMPILER="${CROSS_PATH}/bin/${CROSS_PREFIX}-gcc" \
    -DCMAKE_CXX_COMPILER="${CROSS_PATH}/bin/${CROSS_PREFIX}-g++" \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DCMAKE_C_FLAGS="-march=armv8-a -mtune=cortex-a53 -O3 -flto -ffast-math -pipe -fomit-frame-pointer -DNDEBUG" \
    -DCMAKE_CXX_FLAGS="-march=armv8-a -mtune=cortex-a53 -O3 -flto -ffast-math -pipe -fomit-frame-pointer -DNDEBUG" \
    -DCMAKE_EXE_LINKER_FLAGS="-flto -Wl,--gc-sections -Wl,--strip-all" \
    -DENABLE_JIT=ON \
    -DENABLE_OGLRENDERER=OFF \
    -DENABLE_LTO=ON \
    -DBUILD_QT_SDL=OFF \
    -G "Unix Makefiles"

echo "=== Building with optimizations for handheld ==="
# Use limited parallel jobs to avoid OOM on 1GB RAM
make -j2

echo "=== Build complete! ==="
echo "Binary location: $(pwd)/melonDS"
echo ""
echo "=== Recommended runtime settings for your handheld: ==="
echo "1. Use software renderer (OpenGL disabled)"
echo "2. Limit framerate to 30 FPS or less"
echo "3. Use small audio buffer"
echo "4. Disable JIT if still too slow"
echo "5. Use 1x or 2x screen scaling max"
echo ""
echo "NOTE: This build has no GUI frontend - core library only"
echo "You'll need to create your own frontend or use the library programmatically"