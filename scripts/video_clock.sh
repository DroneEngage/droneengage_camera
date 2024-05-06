#!/bin/bash

ffmpeg -f lavfi -i testsrc=size=1280x720:rate=30 -pix_fmt yuv420p  -c:v rawvideo -f v4l2 /dev/video1