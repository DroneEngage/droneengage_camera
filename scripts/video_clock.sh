#!/bin/bash

# Default device number
VIDEO_DEV=6

# Parse parameter: video device number (e.g. ./video_clock.sh 2 -> /dev/video2)
if [ -n "$1" ]; then
    VIDEO_DEV="$1"
fi

ffmpeg -hide_banner -loglevel warning \
-f lavfi -i testsrc2=size=640x480:rate=120 \
-pix_fmt yuyv422 -c:v rawvideo -f v4l2 /dev/video${VIDEO_DEV}
