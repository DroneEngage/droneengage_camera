#!/bin/bash
sudo apt install v4l2loopback-dkms
sudo modprobe -r v4l2loopback
sudo modprobe v4l2loopback devices=3
ls /sys/devices/virtual/video4linux/

sudo modprobe v4l2loopback devices=3 video_nr=1,2,3 card_label="V-CAM1,V-CAM2,V-CAM3" exclusive_caps=1,1,1

libcamera-vid --width 1280 --height 720 --framerate 30 --codec mjpeg --timeout 0 --output - | ffmpeg -i - -f rawvideo -pix_fmt yuv420p -f v4l2 /dev/video3




sudo apt install gstreamer1.0-tools gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-libcamera

gst-launch-1.0 -v libcamerasrc ! \
"video/x-raw,width=1280,height=720,framerate=30/1,format=YUY2" ! \
videoconvert ! \
v4l2sink device=/dev/video3