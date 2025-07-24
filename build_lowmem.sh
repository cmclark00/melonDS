#!/bin/bash

# melonDS Ultra Low Memory Build Script
# For 1GB RAM handheld with aggressive memory optimization

set -e

echo "=== melonDS Ultra Low Memory Build ==="
echo "Target: 1GB RAM maximum memory usage"

# Create build directory
mkdir -p build-lowmem
cd build-lowmem

# Configure with maximum memory efficiency
cmake .. \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DCMAKE_C_FLAGS="-march=armv8-a -mtune=cortex-a73 -Os -flto -ffunction-sections -fdata-sections -pipe -fomit-frame-pointer -DNDEBUG -DLOW_MEMORY_BUILD" \
    -DCMAKE_CXX_FLAGS="-march=armv8-a -mtune=cortex-a73 -Os -flto -ffunction-sections -fdata-sections -pipe -fomit-frame-pointer -DNDEBUG -DLOW_MEMORY_BUILD" \
    -DCMAKE_EXE_LINKER_FLAGS="-flto -Wl,--gc-sections -Wl,--strip-all -Wl,--as-needed" \
    -DENABLE_JIT=OFF \
    -DENABLE_OGLRENDERER=OFF \
    -DENABLE_LTO=ON \
    -DBUILD_QT_SDL=ON \
    -DUSE_QT6=OFF \
    -G Ninja

echo "=== Building with maximum memory efficiency ==="
# Single threaded build to minimize memory usage during compilation
ninja -j1

echo "=== Build complete! ==="
echo "Binary location: $(pwd)/melonDS"
echo ""
echo "=== Low memory runtime recommendations: ==="
echo "1. JIT disabled for lower memory usage"
echo "2. Use software renderer only"
echo "3. Limit to 15-30 FPS"
echo "4. Close other applications when running"
echo "5. Consider using swap file if needed"