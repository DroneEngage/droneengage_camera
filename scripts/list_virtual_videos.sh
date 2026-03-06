#!/bin/bash

for syspath in /sys/devices/virtual/video4linux/video*; do
    if [ -d "$syspath" ]; then
        # Get the device node name (e.g., video1)
        dev_node=$(basename "$syspath")
        # Read the label assigned to this virtual device
        label=$(cat "$syspath/name")
        
        printf "/dev/%-8s : %s\n" "$dev_node" "$label"
    fi
done