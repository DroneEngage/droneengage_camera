sudo modprobe v4l2loopback video_nr=20


ffmpeg -y -f v4l2 -i /dev/video1 -vcodec copy -f v4l2 /dev/video20