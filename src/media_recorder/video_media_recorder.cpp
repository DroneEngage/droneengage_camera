#include "../common.h"

using namespace uavos;
using namespace uavos::media_recorder::video;


void * _execCommand(void *cmd)
{
    
    std::cout << __FULL_DEBUG__ << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Successfully start _execCommand" << " " << ((const char *)cmd) << _NORMAL_CONSOLE_TEXT_ << std::endl;

    auto pPipe = ::popen(((const char *) cmd), "r");
    if(pPipe == nullptr)
    {
        //throw std::runtime_error("Cannot open pipe");
        return NULL; // cannot be used with webrtc erxception disabled flag
    }

    std::array<char, 256> buffer;

    std::string result;

    while(not std::feof(pPipe))
    {
        auto bytes = std::fread(buffer.data(), 1, buffer.size(), pPipe);
        result.append(buffer.data(), bytes);
    }

    int rc = ::pclose(pPipe);
    if (rc !=0) 
    {
     std::cout << _ERROR_CONSOLE_TEXT_ << "Video Recording exiting error: "  <<  std::to_string(rc) << std::endl;

    }
    return NULL;
}




uavos::media_recorder::video::CVideoRecorder::~CVideoRecorder ()
{
    
}


void uavos::media_recorder::video::CVideoRecorder::init ()
{
    
    std::cout << "init VideoRecorder" << std::endl;

    return ;
}

void uavos::media_recorder::video::CVideoRecorder::processVideoRecording (const Json::Value &jMsg)
{
    // extract command
    
    const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    const int subCommand = cmd["C"].asInt();
    
    // double check cmd.
    if (subCommand != RemoteCommand_RECORDVIDEO) return ;
    
    bool startIfTrue = cmd["Act"].asBool();

    if (startIfTrue == true)
    {
        startVideoRecording (jMsg);
    }
    else
    {
        stopVideoRecording (jMsg);
    }
    
}

void uavos::media_recorder::video::CVideoRecorder::stopVideoRecording (const Json::Value &jMsg)
{
    // extract command
    CWEBRTC_Plugin * cWEBRTC_Plugin;
    cWEBRTC_Plugin = &CWEBRTC_Plugin::getInstance(); 

    const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    
    std::string channelName = cmd["T"].asString();
    
    uavos::stream_webrtc::STRUCT_DEVICE_INFO device_info = cWEBRTC_Plugin->findDeviceInfoByLocalName(channelName.c_str());
    if (device_info.device_num == -1)
    {
        
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: Camera " << channelName.c_str() << " Not Found" << _NORMAL_CONSOLE_TEXT_ << std::endl;
  
        return;
    }

    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "DEBUG: Camera Found " << channelName.c_str() << _NORMAL_CONSOLE_TEXT_ << std::endl;


    #ifdef DEBUG
    std::cout << __FULL_DEBUG__  << _SUCCESS_CONSOLE_BOLD_TEXT_ << "startVideoRecording" << channelName.c_str() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    // Record command that is called in command prompt
    std::ostringstream recCommand;  
    recCommand << "./script_stop_record_video.sh " << device_info.dev_linux_number << " " << device_info.recordFileTimeStamp;
    
    this->startThread(recCommand.str());
    
    device_info.recording = false;
    cWEBRTC_Plugin->updateDeviceInfoByLocalName(channelName.c_str(), device_info);
    
}

/**
 *  Start video recording.
 *  The logic behind video reecording is callig bash script to record video using whatever tool such as 
 *  ffmpeg or gst-launch.
 *  the script should be able to stop itself if it was running.
 * 
 **/
void uavos::media_recorder::video::CVideoRecorder::startVideoRecording (const Json::Value &jMsg)
{
    // extract command
    CWEBRTC_Plugin * cWEBRTC_Plugin;
    cWEBRTC_Plugin = &CWEBRTC_Plugin::getInstance(); 

    const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    
    const std::string channelName = cmd["T"].asString();
    

    uavos::stream_webrtc::STRUCT_DEVICE_INFO device_info = cWEBRTC_Plugin->findDeviceInfoByLocalName(channelName.c_str());
    if (device_info.device_num == -1)
    {
        
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: Camera " << channelName.c_str() << " Not Found" << _NORMAL_CONSOLE_TEXT_ << std::endl;
  
        return;
    }
    
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "DEBUG: Camera Found " << channelName.c_str() << _NORMAL_CONSOLE_TEXT_ << std::endl;

    device_info.recordFileTimeStamp = uavos::util::CHelper::getFileTimeStamp();
    device_info.recording = true;
    cWEBRTC_Plugin->updateDeviceInfoByLocalName(channelName.c_str(), device_info);
    
    std::ostringstream recCommand;  
    recCommand << "./script_start_record_video.sh " << device_info.dev_linux_number << " " << device_info.recordFileTimeStamp;
    
    #ifdef DEBUG
    std::cout << __FULL_DEBUG__  << _SUCCESS_CONSOLE_BOLD_TEXT_ << "RecordCMD:"  << recCommand.str() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    this->startThread(recCommand.str());
    
    
    
    
    return;
    
}


void uavos::media_recorder::video::CVideoRecorder::startImageCapturing (const Json::Value &jMsg)
{
    // extract command
    CWEBRTC_Plugin * cWEBRTC_Plugin;
    cWEBRTC_Plugin = &CWEBRTC_Plugin::getInstance(); 

    const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    const std::string channelName = cmd["a"].asString();
    const int numberOfImages = cmd["b"].asInt();
    const int timeBetweenShots = cmd["c"].asInt();
    //const int distanceBetweenShots = cmd["d"].asInt();  // NOT SUPPORTED

    
    uavos::stream_webrtc::STRUCT_DEVICE_INFO device_info = cWEBRTC_Plugin->findDeviceInfoByLocalName(channelName.c_str());
    if (device_info.device_num == -1)
    {
        
        #ifdef DEBUG
            std::cout << __FULL_DEBUG__  <<  _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: Camera " << channelName.c_str() << " Not Found" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif

        return;
    }

    #ifdef DEBUG
            std::cout << __FULL_DEBUG__  << _SUCCESS_CONSOLE_BOLD_TEXT_ << "DEBUG: Camera Found " << channelName.c_str() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    device_info.recordFileTimeStamp = uavos::util::CHelper::getFileTimeStamp();

    std::ostringstream recCommand;  
    recCommand << "./script_take_image.sh " << device_info.dev_linux_number << " " << device_info.recordFileTimeStamp << " " << numberOfImages << " " << timeBetweenShots;
    
    #ifdef DEBUG
    std::cout << __FULL_DEBUG__  << _SUCCESS_CONSOLE_BOLD_TEXT_ << "RecordCMD:"  << recCommand.str() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    this->startThread(recCommand.str());
    
    //device_info.recording = false;
    cWEBRTC_Plugin->updateDeviceInfoByLocalName(channelName.c_str(), device_info);
    
    return;
}

void uavos::media_recorder::video::CVideoRecorder::startThread (const std::string cmd)
{

    if(pthread_create (&this->m_videoRecorderThread, NULL, _execCommand, (void *)cmd.c_str()))
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Failed to execute record camera" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        return ;
    }

    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Successfully start calling script" << " " << cmd.c_str() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    sleep(1)    ;
}


std::string uavos::media_recorder::video::CVideoRecorder::execCommand(const std::string cmd, int& out_exitStatus)
{
    out_exitStatus = 0;
    auto pPipe = ::popen(cmd.c_str(), "r");
    if(pPipe == nullptr)
    {
        //throw std::runtime_error("Cannot open pipe");
        return std::string(""); // cannot be used with webrtc erxception disabled flag
    }

    std::array<char, 256> buffer;

    std::string result;

    while(not std::feof(pPipe))
    {
        auto bytes = std::fread(buffer.data(), 1, buffer.size(), pPipe);
        result.append(buffer.data(), bytes);
    }

    auto rc = ::pclose(pPipe);

    if(WIFEXITED(rc))
    {
        out_exitStatus = WEXITSTATUS(rc);
    }

    return result;
}

