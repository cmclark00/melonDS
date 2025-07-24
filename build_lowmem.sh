#!/bin/bash

# melonDS Ultra Low Memory Build Script
# For 1GB RAM handheld with aggressive memory optimization

set -e

echo "=== melonDS Ultra Low Memory Build ==="
echo "Target: 1GB RAM maximum memory usage"

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
mkdir -p build-lowmem
cd build-lowmem

# Configure with maximum memory efficiency
cmake .. \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
    -DCMAKE_C_COMPILER="${CROSS_PATH}/bin/${CROSS_PREFIX}-gcc" \
    -DCMAKE_CXX_COMPILER="${CROSS_PATH}/bin/${CROSS_PREFIX}-g++" \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DCMAKE_C_FLAGS="-march=armv8-a -mtune=cortex-a73 -Os -flto -ffunction-sections -fdata-sections -pipe -fomit-frame-pointer -DNDEBUG -DLOW_MEMORY_BUILD" \
    -DCMAKE_CXX_FLAGS="-march=armv8-a -mtune=cortex-a73 -Os -flto -ffunction-sections -fdata-sections -pipe -fomit-frame-pointer -DNDEBUG -DLOW_MEMORY_BUILD" \
    -DCMAKE_EXE_LINKER_FLAGS="-flto -Wl,--gc-sections -Wl,--strip-all -Wl,--as-needed" \
    -DENABLE_JIT=OFF \
    -DENABLE_OGLRENDERER=OFF \
    -DENABLE_LTO=ON \
    -DBUILD_QT_SDL=OFF \
    -G "Unix Makefiles"

echo "=== Building with maximum memory efficiency ==="
# Single threaded build to minimize memory usage during compilation
make -j1

echo "=== Build complete! ==="
echo "Binary location: $(pwd)/melonDS"
echo ""
echo "=== Low memory runtime recommendations: ==="
echo "1. JIT disabled for lower memory usage"
echo "2. Use software renderer only"
echo "3. Limit to 15-30 FPS"
echo "4. Close other applications when running"
echo "5. Consider using swap file if needed"
echo ""
echo "NOTE: This build has no GUI frontend - core library only"
echo "You'll need to create your own frontend or use the library programmatically"