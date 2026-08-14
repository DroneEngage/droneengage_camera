#!/bin/bash

# Default device number
VIDEO_DEV=0

# Parse parameter: video device number (e.g. ./video_colors.sh 6 -> /dev/video6)
if [ -n "$1" ]; then
    VIDEO_DEV="$1"
fi

ffmpeg -f lavfi -i rgbtestsrc -pix_fmt yuv420p  -c:v rawvideo -f v4l2 /dev/video${VIDEO_DEV}
