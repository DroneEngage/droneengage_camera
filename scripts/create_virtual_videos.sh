#!/bin/bash
#sudo apt install v4l2loopback-dkms
sudo modprobe -r v4l2loopback
sudo modprobe v4l2loopback devices=5 video_nr=1,2,3,4,5 card_label="DE-CAM1,SIM-CAM1,DE-TRK,DE-RPI,DE-THERMAL" exclusive_caps=1,1,1,1,1
ls /sys/devices/virtual/video4linux/



