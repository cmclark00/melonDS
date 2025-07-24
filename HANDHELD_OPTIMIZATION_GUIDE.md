# melonDS ARM64 Gaming Handheld Optimization Guide

## Target Hardware
- **Device**: ARM64 Gaming Handheld
- **SoC**: Allwinner H700 (max 2GHz)
- **RAM**: 1GB
- **OS**: Linux 4.9.170 BSP

## Quick Start

1. **Build melonDS optimized for your handheld:**
   ```bash
   ./build_handheld.sh
   ```
   
   For even lower memory usage:
   ```bash
   ./build_lowmem.sh
   ```

   **Note**: These builds create the core melonDS libraries (`libcore.a`, `libteakra.a`) without the Qt GUI frontend to avoid Qt5/Qt6 dependencies during cross-compilation. This gives you the complete DS emulation engine that you can integrate into your own frontend or existing gaming frontend.

2. **Optimize your system (run as root):**
   ```bash
   sudo ./optimize_handheld.sh
   ```

3. **Build the SDL Frontend:**
   ```bash
   ./build_launcher.sh
   ```

4. **Integration Options:**
   - **Option A**: Use the provided SDL frontend (recommended for MUOS)
   - **Option B**: Integrate `libcore.a` into your existing gaming frontend (RetroArch, etc.)
   - **Option C**: Use the library programmatically in your own application

5. **For MUOS Integration:**
   ```bash
   # Copy the SDL frontend to your handheld
   cp build-launcher/melonDS /path/to/handheld/MUOS/emulator/melonds/
   cp launch.sh /path/to/handheld/MUOS/emulator/melonds/
   cp melonDS_muos_launch.sh /path/to/handheld/MUOS/script/
   
   # Replace the DraStic launch script with melonDS
   # Edit your MUOS config to use melonDS_muos_launch.sh for .nds files
   ```

6. **For testing (if you have a native Qt build):**
   ```bash
   mkdir -p ~/.config/melonDS/
   cp handheld_config.ini ~/.config/melonDS/
   ```

## Build Optimizations Explained

### Standard Handheld Build (`build_handheld.sh`)
- **Self-contained**: No need to source cross-compilation environment
- **JIT Enabled**: Uses ARM64 JIT for better performance
- **OpenGL Disabled**: Saves memory and ensures compatibility
- **Aggressive Optimization**: `-O3` with ARM64-specific tuning
- **Link-Time Optimization**: Reduces binary size and improves performance
- **No Qt Dependency**: Builds core libraries only, avoiding Qt5/Qt6 cross-compilation issues

### Low Memory Build (`build_lowmem.sh`)
- **JIT Disabled**: Reduces memory usage at cost of performance
- **Size Optimization**: `-Os` for smallest possible binary
- **Function/Data Sections**: Better dead code elimination

### SDL Frontend (`build_launcher.sh`)
- **Minimal Dependencies**: Only requires SDL2, no Qt5/Qt6
- **Direct Boot**: Boots straight into games like DraStic
- **Optimized for Handhelds**: 30 FPS target, efficient rendering
- **Simple Controls**: Keyboard input mapped to DS buttons
- **MUOS Compatible**: Drop-in replacement for DraStic

## Performance Expectations

### With JIT (Recommended)
- **2D Games**: 45-60 FPS (most games)
- **3D Games**: 20-45 FPS (varies by complexity)
- **Memory Usage**: 150-300MB

### Without JIT (Low Memory)
- **2D Games**: 25-45 FPS
- **3D Games**: 15-30 FPS
- **Memory Usage**: 100-200MB

## Runtime Optimizations

### Critical Settings
1. **Use Software Renderer**: OpenGL disabled for stability
2. **Limit FPS to 30**: Reduces CPU load significantly
3. **Lower Audio Buffer**: 512 samples for less memory usage
4. **Disable WiFi Emulation**: Saves CPU and memory

### System Optimizations
1. **CPU Governor**: Set to "performance" for consistent speed
2. **Memory Management**: Reduced swappiness, optimized cache
3. **I/O Scheduler**: Deadline for better real-time performance
4. **Unnecessary Services**: Disabled to free memory

## Game-Specific Tips

### Best Performance Games
- **2D Platformers**: Mario, Kirby, Metroid
- **Puzzle Games**: Tetris, Professor Layton
- **Turn-based RPGs**: Pokémon, Dragon Quest

### Challenging Games
- **3D Action**: Star Fox, Super Mario 64 DS
- **Racing Games**: Mario Kart DS
- **Real-time Strategy**: Advance Wars

### Settings for Different Game Types

#### 2D Games (Best Performance)
```
MaxFPS=60
FrameSkip=0
HiResolution=0
```

#### 3D Games (Balanced)
```
MaxFPS=30
FrameSkip=1
HiResolution=0
```

#### Complex 3D Games (Maximum Compatibility)
```
MaxFPS=20
FrameSkip=2
HiResolution=0
JIT=disabled (if using low memory build)
```

## Troubleshooting

### Performance Issues
1. **Monitor with**: `./monitor_performance.sh`
2. **Check temperature**: Should stay below 70°C
3. **Memory usage**: Keep below 80% total RAM
4. **CPU frequency**: Watch for thermal throttling

### Common Solutions
- **Stuttering**: Lower FPS limit or enable frame skip
- **Memory errors**: Use low memory build, close other apps
- **Crashes**: Disable JIT, use software renderer only
- **Overheating**: Lower FPS, improve cooling, check for dust

## Advanced Optimizations

### Kernel-Level Optimizations
```bash
# Add to /etc/sysctl.conf for permanent changes
vm.swappiness=10
vm.vfs_cache_pressure=50
vm.overcommit_memory=1
```

### CPU Frequency Scaling
```bash
# Lock to maximum frequency (may increase heat)
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Or use conservative scaling for better thermal management
echo conservative | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

### Storage Optimizations
```bash
# Use deadline I/O scheduler for better real-time performance
echo deadline | sudo tee /sys/block/*/queue/scheduler

# Mount options for better performance (add to /etc/fstab)
# /dev/mmcblk0p1 / ext4 defaults,noatime,nodiratime 0 1
```

## Power Management

### Battery Life Tips
1. **Lower screen brightness**: Significant power savings
2. **Limit FPS to 20-30**: Reduces CPU load
3. **Use CPU governor "powersave"** when not gaming
4. **Close melonDS completely** when not in use

### Thermal Management
1. **Monitor temperature**: Use monitoring script
2. **Improve airflow**: Clean vents, ensure proper positioning
3. **Thermal throttling**: Accept lower performance to prevent overheating
4. **Ambient temperature**: Gaming in cooler environments helps

## ROMs and Performance

### Best Formats
- **NDS files**: Standard format, good compatibility
- **Compressed ROMs**: 7z/zip can save storage but may impact loading

### ROM Size Impact
- **Small ROMs (< 32MB)**: Minimal performance impact
- **Large ROMs (> 128MB)**: May cause longer loading times
- **Huge ROMs (> 256MB)**: Consider using faster storage

## SDL Frontend Controls

### Default Input Mapping
- **Arrow Keys**: D-Pad (Up, Down, Left, Right)
- **A Key**: DS A Button
- **S Key**: DS B Button
- **Space**: Start Button
- **Enter**: Select Button
- **Q Key**: L Shoulder Button
- **W Key**: R Shoulder Button
- **X Key**: DS X Button
- **Z Key**: DS Y Button
- **ESC**: Exit emulator

### Usage
```bash
# Direct ROM launch
./melonDS /path/to/game.nds

# Using the wrapper script
./launch.sh /path/to/game.nds
```

## Monitoring and Maintenance

### Use the Performance Monitor
```bash
./monitor_performance.sh
```

### Key Metrics to Watch
- **Memory usage**: Should stay below 800MB
- **CPU temperature**: Keep below 70°C
- **CPU frequency**: Watch for throttling
- **melonDS process memory**: Monitor for memory leaks

### Regular Maintenance
1. **Clean up saves/states**: Remove old files
2. **Update ROM database**: Keep ROMList.bin current
3. **Check for updates**: Newer melonDS versions may perform better
4. **Storage cleanup**: Free space improves performance

## Conclusion

With these optimizations, your 1GB ARM64 handheld should run many DS games at playable framerates. The SDL frontend provides a DraStic-like experience that boots directly into games, making it perfect for MUOS and other handheld operating systems.

### Key Benefits:
- **No Qt Dependencies**: Builds and runs without complex GUI libraries
- **Direct Boot**: Launches games immediately like commercial emulators
- **MUOS Integration**: Drop-in replacement for DraStic
- **Optimized Performance**: Tuned for 1GB RAM handheld devices
- **Simple Setup**: Just build and copy files to your device

Focus on 2D games and simpler 3D titles for the best experience. The key is finding the right balance between performance and thermal management for your specific device and environment.

Remember: Every handheld is different, so experiment with settings to find what works best for your specific device and use case!