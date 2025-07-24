#!/bin/bash

# System optimization script for melonDS on ARM64 gaming handheld
# Run as root or with sudo

echo "=== Optimizing ARM64 Handheld for melonDS ==="

# Set CPU governor to performance for consistent speeds
echo "Setting CPU governor to performance..."
if [ -d /sys/devices/system/cpu/cpu0/cpufreq ]; then
    echo performance | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
fi

# Adjust virtual memory settings for low RAM
echo "Optimizing memory management..."
echo 10 > /proc/sys/vm/swappiness  # Reduce swapping
echo 1 > /proc/sys/vm/overcommit_memory  # Be more aggressive with memory allocation
echo 50 > /proc/sys/vm/vfs_cache_pressure  # Reduce file system cache pressure

# Set I/O scheduler to deadline for better real-time performance
echo "Setting I/O scheduler..."
for disk in /sys/block/*/queue/scheduler; do
    if [ -f "$disk" ]; then
        echo deadline > "$disk" 2>/dev/null || echo mq-deadline > "$disk" 2>/dev/null || true
    fi
done

# Disable unnecessary services to free up memory
echo "Disabling unnecessary services..."
services_to_stop=(
    "bluetooth"
    "cups"
    "avahi-daemon"
    "NetworkManager"  # Only if using static IP or simple networking
)

for service in "${services_to_stop[@]}"; do
    if systemctl is-active --quiet "$service"; then
        echo "Stopping $service..."
        systemctl stop "$service" 2>/dev/null || true
    fi
done

# Create a swap file if none exists and system has less than 2GB RAM
total_ram=$(free -m | awk 'NR==2{print $2}')
if [ "$total_ram" -lt 2048 ]; then
    if [ ! -f /swapfile ]; then
        echo "Creating 1GB swap file for low memory system..."
        fallocate -l 1G /swapfile
        chmod 600 /swapfile
        mkswap /swapfile
        swapon /swapfile
        echo '/swapfile none swap sw 0 0' >> /etc/fstab
    fi
fi

# Set real-time scheduling priority
echo "Setting up real-time scheduling..."
echo '@audio - rtprio 95' >> /etc/security/limits.conf
echo '@audio - memlock unlimited' >> /etc/security/limits.conf

echo "=== Optimization complete! ==="
echo ""
echo "To run melonDS with optimal settings:"
echo "1. Close all unnecessary applications"
echo "2. Run: nice -n -10 ./melonDS"
echo "3. Use the provided handheld_config.ini"
echo ""
echo "For best performance:"
echo "- Use software renderer"
echo "- Limit FPS to 30 or less"
echo "- Use small ROM files when possible"
echo "- Consider overclocking if thermal management allows"