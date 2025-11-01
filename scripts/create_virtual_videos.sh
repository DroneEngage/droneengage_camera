#!/bin/bash
sudo apt install v4l2loopback-dkms
sudo modprobe -r v4l2loopback
sudo modprobe v4l2loopback devices=3
sudo modprobe v4l2loopback devices=5 video_nr=1,2,3,4,5 card_label="DE-CAM1,DE-CAM2,DE-TRK,DE-RPI,DE-THERMAL" exclusive_caps=1,1,1,1,1
ls /sys/devices/virtual/video4linux/


libcamera-vid --width 1280 --height 720 --framerate 30 --codec mjpeg --timeout 0 --output - | ffmpeg -i - -f rawvideo -pix_fmt yuv420p -f v4l2 /dev/video3




sudo apt install gstreamer1.0-tools gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-libcamera

gst-launch-1.0 -v libcamerasrc ! "video/x-raw,width=1280,height=720,framerate=30/1,format=YUY2" ! videoconvert ! v4l2sink device=/dev/video3

SONY:
rpicam-hello -t 0s --post-process-file /usr/share/rpi-camera-assets/imx500_mobilenet_ssd.json --viewfinder-width 1920 --viewfinder-height 1080 --framerate 30
gst-launch-1.0 -v libcamerasrc ! "video/x-raw,width=640,height=480,framerate=30/1,format=YUY2" ! videoconvert ! v4l2sink device=/dev/video3
rpicam-raw -t 0 --width 640 --height 480 --framerate 30 --codec yuv420 -o --post-process-file /usr/share/rpi-camera-assets/imx500_mobilenet_ssd.json - | ffmpeg -f rawvideo -pixel_format yuv420p -video_size 640x480 -i - -f v4l2 -pixel_format yuv420p /dev/video3


## WORKS on RPI-4

rpicam-vid -t 0s --post-process-file /usr/share/rpi-camera-assets/imx500_mobilenet_ssd.json --viewfinder-width 800 --viewfinder-height 600 --framerate 30 --listen -o tcp://0.0.0.0:8000

rpicam-vid -t 0s --post-process-file /usr/share/rpi-camera-assets/imx500_posenet.json --viewfinder-width 800 --viewfinder-height 600 --framerate 30 --listen -o tcp://0.0.0.0:8000


ffmpeg -f h264 -i tcp://127.0.0.1:8000 \
-vf "fps=30,format=yuv420p,scale=800:600" \
-f v4l2 /dev/video3



### WORKS ON RPI-5 (RESOLUTION 640x480) is a MUST !!!

rpicam-vid -t 0 --vflip=1 --width 640 --height 480 --framerate 30 --codec yuv420 -o - --post-process-file /usr/share/rpi-camera-assets/imx500_mobilenet_ssd.json | ffmpeg -f rawvideo -pixel_format yuv420p -video_size 640x480 -i - -f v4l2 -pixel_format yuv420p /dev/video10


### NO LOG FROM BOTH RPICAM & FFMPEG
rpicam-vid -t 0 --vflip=1 --width 640 --height 480 --framerate 30 --codec yuv420 --info-text "" -o - --post-process-file /usr/share/rpi-camera-assets/imx500_mobilenet_ssd.json | ffmpeg -f rawvideo -pixel_format yuv420p -video_size 640x480 -i - -f v4l2 -pixel_format yuv420p /dev/video10 -loglevel quiet


