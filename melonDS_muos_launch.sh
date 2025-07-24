#!/bin/sh

. /opt/muos/script/var/func.sh

NAME=$1
CORE=$2
FILE=${3%/}

(
	LOG_INFO "$0" 0 "Content Launch" "DETAIL"
	LOG_INFO "$0" 0 "NAME" "$NAME"
	LOG_INFO "$0" 0 "CORE" "$CORE"
	LOG_INFO "$0" 0 "FILE" "$FILE"
) &

HOME="$(GET_VAR "device" "board/home")"
export HOME

SETUP_SDL_ENVIRONMENT

SET_VAR "system" "foreground_process" "melonDS"

EMUDIR="$(GET_VAR "device" "storage/rom/mount")/MUOS/emulator/melonds"

chmod +x "$EMUDIR"/melonDS
cd "$EMUDIR" || exit

/opt/muos/script/mux/track.sh "$NAME" "$CORE" "$FILE" start

# Set optimal CPU governor for performance
echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

# Run melonDS with the ROM file
HOME="$EMUDIR" SDL_ASSERT=always_ignore nice -n -10 ./melonDS "$FILE"

# Restore CPU governor
echo ondemand > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

/opt/muos/script/mux/track.sh "$NAME" "$CORE" "$FILE" stop

unset SDL_HQ_SCALER SDL_ROTATION SDL_BLITTER_DISABLED 