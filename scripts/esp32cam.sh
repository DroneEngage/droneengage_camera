sudo apt install raspberrypi-kernel-headers
sudo apt install v4l2loopback-dkms
sudo modprobe v4l2loopback devices=3
sudo apt install ffmpeg
ffmpeg -i http://192.168.1.139:81/stream -pix_fmt yuyv422 -f v4l2 /dev/video2
