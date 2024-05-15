#include <stdio.h>
#include <stdlib.h>
#include <iostream>  
#include <string.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <fcntl.h>
// v4l includes
#include <linux/videodev2.h>

#include "../common.h"


//OPENCV Capturer check this : https://sourcey.com/articles/webrtc-custom-opencv-video-capture

// rtc::scoped_refptr<webrtc::VideoCaptureModule> CSource::GetVideoCapturerByIndex(const char * device_id) {

//     std::cout << "GetVideoCapturerByIndex called" << std::endl;
//     std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> video_info(webrtc::VideoCaptureFactory::CreateDeviceInfo());

//     rtc::scoped_refptr<webrtc::VideoCaptureModule>  capturerModule = webrtc::VideoCaptureFactory::Create(device_id);

//     if (!capturerModule) {
//         std::cout << "capturerModule is null" << std::endl;
//     return nullptr;
//     }

//     webrtc::VideoCaptureCapability capability_;
//     video_info->GetCapability(capturerModule->CurrentDeviceName(), 0, capability_);
    
//     std::cout << "height:" << capability_.height << " width:" << capability_.width << std::endl;

//     return capturerModule;
// }


using namespace de::stream_webrtc;



/**
 * @brief get n where n is /dev/video(n) of camera as this information is not returned by webrtc.
 * @details This is important for video recording and camera configuration selection.
 * @param device_name 
 * @return int 
 */
int de::stream_webrtc::CSource::GetDeviceNumber (std::string device_name)
{
    // Travel through /dev/video [0-63]
    std::string device_name_instance;
        
    char device[20];
    int fd = -1;
    struct v4l2_capability cap;
    for (int n = 0; n < 64; n++) {
        sprintf(device, "/dev/video%d", n);
        if ((fd = open(device, O_RDONLY)) != -1) {
        // query device capabilities and make sure this is a video capture device
        if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0 ||
            !(cap.device_caps & V4L2_CAP_VIDEO_CAPTURE)) {
            close(fd);
            continue;
        }
        }
        else
        {
            continue;
        }
        

        // // query device capabilities
        // if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        //     RTC_LOG(LS_INFO) << "error in querying the device capability for device "
        //                     << device << ". errno = " << errno;
        //     close(fd);
        //     return -1;
        // }

        close(fd);

        char camera_name[64];
        memcpy(camera_name, cap.card, sizeof(cap.card));
        device_name_instance = std::string(camera_name);
        
        //if (device_name.compare(device_name_instance)==-1) return n;
        if (strcmp (device_name.c_str(),camera_name)==0) return n;
    }

    return -1;
}

int de::stream_webrtc::CSource::GetDevices (std::vector<STRUCT_DEVICE_INFO> &devicesInfo, const int deviceCount) {
    
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> video_info(webrtc::VideoCaptureFactory::CreateDeviceInfo());
    
    if (!video_info) return 0;
    const int numDevices = video_info->NumberOfDevices();
    int counter = numDevices<deviceCount?numDevices:deviceCount;
    de::util::CHelper::initRandom();
    
    for (int i=0; i<counter; ++i)
    {
        const uint32_t kSize = 256;
        char name[kSize] = {0};
        char id[kSize] = {0};
        char pid[kSize] = {0};
        STRUCT_DEVICE_INFO deviceInfo;
        // GetDeviceName gets /dev/video[i] 
        if (video_info->GetDeviceName(i, name, kSize, id, kSize, pid, kSize) != -1) 
        {
            deviceInfo.device_num   = i; 
            deviceInfo.device_name  = std::string( name ); //e.g. "Dummy video device (0x0000)"
            deviceInfo.device_id    = std::string( id );   //e.g  "platform:v4l2loopback-000"
            deviceInfo.product_id   = std::string( pid );
            deviceInfo.local_name   = std::string( name ) + "#" + std::to_string(i); // id is added as similar cameras have similar names.
            deviceInfo.unique_name  = de::util::CHelper::getShortSemiGUID();
            deviceInfo.dev_linux_number = GetDeviceNumber (deviceInfo.device_name); // before create source to avoid permission problems.
            deviceInfo.capturer     = std::shared_ptr<VideoDevCapturerComposite>(VideoDevCapturerComposite::Create(1024,720,30,deviceInfo.device_id.c_str()));
            deviceInfo.active       = 0;
            deviceInfo.selected     = false;
            deviceInfo.recording    = false;
            devicesInfo.push_back(deviceInfo);
            
            #ifdef DEBUG
            std::cout << _TEXT_BOLD_HIGHTLITED_ << "-------------------- /dev/video" << _INFO_BOLD_CONSOLE_TEXT << deviceInfo.dev_linux_number << std::endl;
            std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_  << "device_num: " << _INFO_BOLD_CONSOLE_TEXT << deviceInfo.device_num << std::endl;
            std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_  << "device_name: " << _INFO_BOLD_CONSOLE_TEXT<< deviceInfo.device_name << std::endl;
            std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_  << "device_id: " << _INFO_BOLD_CONSOLE_TEXT<< deviceInfo.device_id << std::endl;
            std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_  << "product_id: " << _INFO_BOLD_CONSOLE_TEXT<< deviceInfo.product_id << std::endl;
            std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_  << "local_name: " << _INFO_BOLD_CONSOLE_TEXT<< deviceInfo.local_name << std::endl;
            std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_  << "unique_name: " << _INFO_BOLD_CONSOLE_TEXT<< deviceInfo.unique_name << std::endl;
            std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_  << "recording: " << _INFO_BOLD_CONSOLE_TEXT<< deviceInfo.recording << _NORMAL_CONSOLE_TEXT_ << std::endl;
            
            #endif
           
        }
    }
    return counter;
}

int de::stream_webrtc::CSource::GetVideoSourcesCount(void)
{
    std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> video_info(webrtc::VideoCaptureFactory::CreateDeviceInfo());
 
    if (video_info) return video_info->NumberOfDevices();

    return 0;
}



