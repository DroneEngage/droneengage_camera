#!/bin/bash

ffmpeg -f lavfi -i rgbtestsrc -pix_fmt yuv420p  -c:v rawvideo -f v4l2 /dev/video0
