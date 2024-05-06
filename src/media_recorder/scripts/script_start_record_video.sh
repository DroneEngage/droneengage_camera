#!/bin/bash

# This script is called from the app to provide flexible way of recording videos.


# parameter #1 is video driver
# parameter #2 is file name suffix... SHOULD BE UNIQUE... file date.

echo "output"
echo "Params:" $#
echo "DRIVE:" $1
echo "DATE:" $2
if [ "$#" -eq 1 ]
then
    exit -1
fi
DRIVE=$1
DATE=$2

# kill me if I am still running
CMD="ffmpeg -stdin -y -f v4l2 -i /dev/video$DRIVE -codec:v libx264 -qp 0"
echo "kill $(ps aux | grep "$CMD" | awk '{print $2}')"
sleep .5 
$CMD  cam$DRIVE_$DATE.mp4
  

#echo "$CMD  cam$DRIVE_$DATE.mp4"
