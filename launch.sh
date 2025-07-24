#!/bin/sh

# melonDS launcher wrapper
# Compatible with DraStic launch pattern

ROM_FILE="$1"

if [ -z "$ROM_FILE" ]; then
    echo "Usage: $0 <rom_file>"
    echo "Example: $0 /path/to/game.nds"
    exit 1
fi

if [ ! -f "$ROM_FILE" ]; then
    echo "Error: ROM file not found: $ROM_FILE"
    exit 1
fi

# Set CPU governor to performance mode for better emulation
if [ -w /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ]; then
    echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
fi

# Launch melonDS
exec ./melonDS "$ROM_FILE" 