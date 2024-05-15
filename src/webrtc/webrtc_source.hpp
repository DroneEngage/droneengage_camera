
#ifndef CSOURCE_H
#define CSOURCE_H

namespace de
{

namespace stream_webrtc
{

struct device_info {
    int device_num;         // device number in webrtc which is device index.
    int dev_linux_number;   // represents dev/video%d
    std::string device_name;
    std::string local_name;
    std::string device_id;
    std::string product_id;
    std::string unique_name;
    std::shared_ptr<de::stream_webrtc::VideoDevCapturerComposite> capturer;
    bool selected;          // some can be skipped and not listed as available cam.
    int active;             // already streaming
    bool recording;
    std::string recordFileTimeStamp;
} ; 

typedef struct device_info STRUCT_DEVICE_INFO;

class CSource
{
    public:
        //static rtc::scoped_refptr<webrtc::VideoCaptureModule> GetVideoCapturerByIndex(const char * device_id);
        static int GetDevices (std::vector<STRUCT_DEVICE_INFO> &deviceInfo, const int deviceCount);
        static int GetVideoSourcesCount (void);

    protected:
        static int GetDeviceNumber (std::string device_name);

};

}
}
#endif