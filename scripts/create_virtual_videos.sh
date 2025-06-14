#!/bin/bash
sudo modprobe -r v4l2loopback
sudo modprobe v4l2loopback devices=3
ls /sys/devices/virtual/video4linux/
