#!/bin/bash

# Performance monitoring script for melonDS on ARM64 handheld

echo "=== melonDS Performance Monitor ==="
echo "Press Ctrl+C to stop monitoring"
echo ""

# Function to get CPU temperature (if available)
get_cpu_temp() {
    if [ -f /sys/class/thermal/thermal_zone0/temp ]; then
        temp=$(cat /sys/class/thermal/thermal_zone0/temp)
        echo "$((temp / 1000))°C"
    else
        echo "N/A"
    fi
}

# Function to get current CPU frequency
get_cpu_freq() {
    if [ -f /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq ]; then
        freq=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq)
        echo "$((freq / 1000)) MHz"
    else
        echo "N/A"
    fi
}

# Monitor loop
while true; do
    # Clear screen and show header
    clear
    echo "=== melonDS ARM64 Handheld Performance Monitor ==="
    echo "Time: $(date)"
    echo ""
    
    # System information
    echo "=== System Status ==="
    echo "CPU Temperature: $(get_cpu_temp)"
    echo "CPU Frequency: $(get_cpu_freq)"
    echo ""
    
    # Memory usage
    echo "=== Memory Usage ==="
    free -h | head -n 2
    echo ""
    
    # CPU usage
    echo "=== CPU Usage (per core) ==="
    top -bn1 | grep "Cpu" | head -n 4
    echo ""
    
    # melonDS process info if running
    if pgrep -f melonDS > /dev/null; then
        echo "=== melonDS Process Info ==="
        ps aux | grep -E "(melonDS|PID)" | grep -v grep
        echo ""
        
        # Get detailed memory info for melonDS
        melonds_pid=$(pgrep -f melonDS | head -n 1)
        if [ -n "$melonds_pid" ]; then
            echo "=== melonDS Memory Details ==="
            cat /proc/$melonds_pid/status | grep -E "(VmRSS|VmSize|VmHWM)"
            echo ""
        fi
    else
        echo "=== melonDS not currently running ==="
        echo ""
    fi
    
    # Storage space
    echo "=== Storage Usage ==="
    df -h / | tail -n 1
    echo ""
    
    # Load average
    echo "=== System Load ==="
    uptime
    echo ""
    
    # Show thermal throttling status if available
    if [ -f /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq ] && [ -f /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq ]; then
        cur_freq=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq)
        max_freq=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq)
        throttle_percent=$((cur_freq * 100 / max_freq))
        echo "=== Thermal Status ==="
        echo "CPU running at ${throttle_percent}% of max frequency"
        if [ $throttle_percent -lt 90 ]; then
            echo "⚠️  Possible thermal throttling detected!"
        fi
        echo ""
    fi
    
    echo "=== Performance Tips ==="
    echo "• Lower FPS limit if CPU usage is high"
    echo "• Use software renderer for stability"
    echo "• Close other apps if memory usage > 80%"
    echo "• Check temperature - consider cooling if > 70°C"
    echo ""
    
    sleep 2
done