#!/bin/bash

# --- Configuration ---
NUM_DEVICES=8
VIDEO_NR="2,3,4,5,6,7,8,9"
CARD_LABELS="DE-CAM1,DE-CAM2,DE-TRK,DE-RPI,DE-THERMAL,DE-AI,DE-GIMBAL,SIM-CAM1"
EXCLUSIVE_CAPS="1,1,1,1,1,1,1,1"

# Define color codes
RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
NC='\033[0m'

echo -e "${BLUE}--- v4l2loopback Device Manager ---${NC}"

# 1. Check if the module is already loaded
if lsmod | grep -q v4l2loopback; then
    echo -e "${YELLOW}Module v4l2loopback is already loaded.${NC}"
    
    # Check if the current loaded labels match our target
    # This avoids flickering/disconnecting if the setup is already correct
    current_labels=$(cat /sys/module/v4l2loopback/parameters/card_label 2>/dev/null)
    
    if [[ "$current_labels" == "$CARD_LABELS" ]]; then
        echo -e "${GREEN}Configuration matches, but forcing reload to reset device state...${NC}"
        echo -e "${YELLOW}Note: This will fail if any app (OBS, Zoom, etc.) is using the cameras.${NC}"
        
        sudo modprobe -r v4l2loopback
        if [ $? -ne 0 ]; then
            echo -e "${RED}Error: Could not unload v4l2loopback. Is a camera in use?${NC}"
            exit 1
        fi
    else
        echo -e "${RED}Configuration mismatch detected. Reloading module...${NC}"
        echo -e "${YELLOW}Note: This will fail if any app (OBS, Zoom, etc.) is using the cameras.${NC}"
        
        sudo modprobe -r v4l2loopback
        if [ $? -ne 0 ]; then
            echo -e "${RED}Error: Could not unload v4l2loopback. Is a camera in use?${NC}"
            exit 1
        fi
    fi
fi

# 2. Load the module with the specified parameters
if ! lsmod | grep -q v4l2loopback; then
    echo -e "${YELLOW}Applying configuration for ${NUM_DEVICES} devices...${NC}"
    sudo modprobe v4l2loopback \
        devices=${NUM_DEVICES} \
        video_nr=${VIDEO_NR} \
        card_label="${CARD_LABELS}" \
        exclusive_caps=${EXCLUSIVE_CAPS}
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Error: Failed to load v4l2loopback module.${NC}"
        exit 1
    fi
    echo -e "${GREEN}Module loaded successfully.${NC}"
fi

# 3. Verification Table
echo -e "\n${BLUE}Virtual Device Mapping:${NC}"
echo "------------------------------------------"
printf "%-15s | %-20s\n" "Device Node" "Card Label"
echo "------------------------------------------"

# Split the VIDEO_NR string into an array for checking
IFS=',' read -ra ADDR <<< "$VIDEO_NR"
for i in "${ADDR[@]}"; do
    dev_path="/dev/video$i"
    if [ -e "$dev_path" ]; then
        # Fetch the label directly from the kernel name attribute
        label=$(cat /sys/class/video4linux/video$i/name 2>/dev/null)
        printf "${GREEN}%-15s${NC} | %-20s\n" "$dev_path" "$label"
    else
        printf "${RED}%-15s${NC} | %-20s\n" "$dev_path" "MISSING"
    fi
done
echo "------------------------------------------"