#!/bin/bash

BOARD="OpenRB-150:samd:OpenRB-150"

PORT=$(ls /dev/ttyACM* 2>/dev/null | grep -i "OpenRB-150" || ls /dev/ttyACM* 2>/dev/null | head -n 1)

if [ -z "$PORT" ]; then
    echo "No suitable port found. Please check device connection."
    exit 1
fi

echo "Compiling sketch..."
if arduino-cli compile --fqbn $BOARD; then
    echo -e "${GREEN}Compilation successful${NC}"
else
    echo -e "${RED}Compilation failed${NC}"
    exit 1
fi

echo "Uploading sketch..."
echo "Using port: $PORT"
if arduino-cli upload -p $PORT --fqbn $BOARD; then
    echo -e "${GREEN}Upload successful${NC}"
else
    echo -e "${RED}Upload failed${NC}"
    exit 1
fi