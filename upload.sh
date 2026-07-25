#!/bin/bash

set -e

# --- CONFIGURE ---
ELF_FILE=build_pico/picorx.elf  # path to ELF, pass as first argument
SERIAL_PORT="$2"  # optional, defaults to first ACM device

if [ -z "$ELF_FILE" ]; then
    echo "Usage: $0 <elf-file> [serial-port]"
    exit 1
fi

# Auto-detect serial port if not specified
if [ -z "$SERIAL_PORT" ]; then
    SERIAL_PORT=$(ls /dev/ttyACM* | head -n1)
fi

if [ -e "$SERIAL_PORT" ]; then
    echo "Using serial port: $SERIAL_PORT"
    echo "ELF file: $ELF_FILE"

    # --- Send reset command ---
    echo "Sending reset command..."
    echo -n "ZR;" > "$SERIAL_PORT"
fi


# --- Wait for device to enter BOOTSEL ---
echo "Waiting for Pico to appear in BOOTSEL..."
sleep 3  # adjust if needed

# --- Flash ELF using picotool ---
echo "Uploading firmware..."
sudo picotool load "$ELF_FILE"
sudo picotool reboot

echo "Done!"
