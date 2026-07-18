#!/bin/bash

ffmpeg -f lavfi -i testsrc=size=1280x720:rate=15 -pix_fmt yuv420p  -c:v rawvideo -f v4l2 /dev/video1

ffmpeg -f lavfi -i testsrc=size=320x200:rate=30 -pix_fmt yuv420p  -c:v rawvideo -f v4l2 /dev/video2


# BANNER
ffmpeg -hide_banner -loglevel warning \
-f lavfi -i testsrc2=size=640x480:rate=120 \
-pix_fmt yuyv422 -c:v rawvideo -f v4l2 /dev/video6