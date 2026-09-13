#!/bin/bash

# --- Odroid-tuned v4l2loopback Device Manager ---
# Variant of sh_camera_create_named_vc.sh for Odroid SBCs (N2+, C4, XU4, M1).
#
# Why a separate script:
#   Odroid boards expose real camera modules and USB webcams on the low
#   /dev/videoX nodes (typically 0..9). The desktop script reserves
#   video_nr=2..9, which clashes with real Odroid cameras. This variant
#   shifts the virtual devices up to video_nr=10..17 so they never collide
#   with real hardware, while keeping the SAME card labels — so downstream
#   modules (de_precland, de_camera) that resolve devices by name
#   (e.g. "DE-RPI", "DE-TRK") work unchanged.
#
# If your Odroid exposes more than 10 real video nodes, raise VIDEO_NR
# below accordingly (e.g. start at 20).

# --- Configuration ---
NUM_DEVICES=8
VIDEO_NR="10,11,12,13,14,15,16,17"
CARD_LABELS="DE-CAM1,DE-CAM2,DE-TRK,DE-RPI,DE-THERMAL,DE-AI,DE-GIMBAL,SIM-CAM1"
EXCLUSIVE_CAPS="1,1,1,1,1,1,1,1"

# Define color codes
RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
NC='\033[0m'

echo -e "${BLUE}--- v4l2loopback Device Manager (Odroid) ---${NC}"

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
    echo -e "${YELLOW}Applying configuration for ${NUM_DEVICES} devices on /dev/video${VIDEO_NR%%,*}..${VIDEO_NR##*,}...${NC}"
    sudo modprobe v4l2loopback \
        devices=${NUM_DEVICES} \
        video_nr=${VIDEO_NR} \
        card_label="${CARD_LABELS}" \
        exclusive_caps=${EXCLUSIVE_CAPS}

    if [ $? -ne 0 ]; then
        echo -e "${RED}Error: Failed to load v4l2loopback module.${NC}"
        echo -e "${YELLOW}Hint: if video_nr=${VIDEO_NR} is taken by real Odroid cameras, edit VIDEO_NR at the top of this script.${NC}"
        exit 1
    fi
    echo -e "${GREEN}Module loaded successfully.${NC}"
fi

# 3. Verification Table
echo -e "\n${BLUE}Virtual Device Mapping (Odroid):${NC}"
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
