#!/bin/bash

# This script is called from the app to provide flexible way of taking image.


# parameter #1 is video driver
# parameter #2 is file name suffix... SHOULD BE UNIQUE... file date.
# parameter #3 is numberOfImages.
# parameter #4 is timeBetweenShots in seconds.


echo "output"
echo "Params:" $#
echo "DRIVE:" $1
echo "DATE:" $2
echo "TOTAL_IMAGES:" $3
echo "TIME_BETWEEN_IMAGES:" $4

if [ "$#" -eq 1 ]
then
    exit -1
fi

DRIVE=$1
DATE=$2
TOTAL_IMAGES=$3
TIME_BETWEEN_IMAGES=$4

echo $(eval echo "$TOTAL_IMAGES")
for i in $(eval echo "{1..$TOTAL_IMAGES}")
    do
    # # kill me if I am still running
    CMD="ffmpeg -y -f v4l2 -i /dev/video$DRIVE -vframes 1 -q:v 2 img${DRIVE}_${DATE}_num_${i}_of_${TOTAL_IMAGES}.jpg"

    $CMD
    sleep $TIME_BETWEEN_IMAGES
    done 

#CMD="ffmpeg -f v4l2 -i /dev/video$DRIVE -vf \"select=eq(mod(n\, $INTERVAL)\, 0\), setpts=N/$INTERVAL/TB\" -q:v 1 "


# SEC=5
# INTERVAL=$((24*$SEC))
# ffmpeg -f v4l2 -i /dev/video2 -vf "select=between(mod(n\, 24)\, 0\, 7), setpts=N/24/TB" -q:v 1 output_%02d.jpg


# ffmpeg -f v4l2 -i /dev/video2  -vf "select='not(mod(n,5))'"  -q:v 1 output_%02d.jpg