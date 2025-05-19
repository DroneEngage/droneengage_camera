# Script Section

## Creating Virtual Video DriversCreating Virtual Video Drivers

This section has script that you can use to test your video capabilities on your desktop without the need of actual camera.

you need to install [v4l2loopback](https://github.com/umlaeute/v4l2loopback "v4l2loopback")  which allows you to create virtual camera video drivers.

Then you can run the following command to create 3 video drivers.

`sudo modprobe v4l2loopback devices=3`

if you do not have camera on your device then you will find devices **/dev/video0**** /dev/video1**  **/dev/video2 ** are created. Exact device number will change if you already have camera on your computer. so it is recommended to run 
`ls /dev/video*`  before and after executing v4l2loopback command.

## Sending Sample Videos

you can use ffmpeg to send clock and colors using the following commands.

`ffmpeg -f lavfi -i testsrc=size=1280x720:rate=30 -pix_fmt yuv420p  -c:v rawvideo -f v4l2 /dev/video1`

[![color_clock](https://github.com/DroneEngage/droneengage_camera/blob/master/res/clock_sample.png?raw=true)](https://github.com/DroneEngage/droneengage_camera/blob/master/res/clock_sample.png?raw=true)

or 

`ffmpeg -f lavfi -i rgbtestsrc -pix_fmt yuv420p  -c:v rawvideo -f v4l2 /dev/video0`

[![color_rgb](https://github.com/DroneEngage/droneengage_camera/blob/master/res/color_sample.png?raw=true)](https://github.com/DroneEngage/droneengage_camera/blob/master/res/color_sample.png?raw=true)

Now you can set your **config.module.json** to access these video drivers and test it using WebClient.

