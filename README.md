# DroneEngage Camera Module

This module uses native webrtc to stream videos from multiple camera to [WebClient](https://github.com/DroneEngage/droneengage_webclient "Weblient").

# Compile The Code

## for X86

first you need webrtc compiled code to get libwebrtc.a 
you need to use branch 

`git ls-remote https://chromium.googlesource.com/external/webrtc --heads branch-heads/4606`

this is an old branch and you need **Ubuntu 18.04** to build it.

Once you have **libwebrtc.a ** just put it in  **/lib/webrtc94-local/lib/x64/Debug** and run 

you can download a compiled version from [here](https://drive.google.com/file/d/10KvinexvvDRUgd6afGLAbMCgCr_h8yM6/view?usp=sharing "here")
`make debug`

## for Arm

use BUILD.gn file in this project to overwrite the example BUILD.gn in webrtc source, and add the current project file inside **src** in a folder called de_camera similar to:

/home/webrtc/webrtc/webrtc-checkout/src/examples/de_camera


[![de_camera](https://github.com/DroneEngage/droneengage_camera/blob/master/res/virtual_machine_de_camera.png?raw=true)](https://github.com/DroneEngage/droneengage_camera/blob/master/res/virtual_machine_de_camera.png?raw=true)

then compile webrtc and it will compile project de_camera as part of the project.
then you can use it in RPI or JetsonNano

## To Install Scripts for Camera

Please check [Script Section](https://github.com/DroneEngage/droneengage_camera/blob/master/scripts/README.md "Script Section")
