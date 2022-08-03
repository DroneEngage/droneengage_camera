#ifndef VIDEORECORDER_H
#define VIDEORECORDER_H


namespace uavos
{
namespace stream_webrtc
{
    

class  CVideoRecording 
{
        
    public:

        bool startRecording();
        bool isRecording();
        bool stopRecording();
        bool screenShot(const uint &image_count, const uint &image_duration);
    
    protected:
        int printPlane(const uint8_t* buf,
               int width,
               int height,
               int stride);
        int printVideoFrame(const webrtc::VideoFrame& frame);
        int saveFrameAsRGB(webrtc::VideoFrame& frame);


    protected:
        bool m_is_video_recording = false;
        uint m_image_count = 0;
        uint m_image_duration;
        std::string m_video_file_name;
        FILE *m_video_handler = nullptr;

    private:
        unsigned char* createBitmapFileHeader (const uint& height, const uint& stride);
        unsigned char* createBitmapInfoHeader (const uint&  height, const uint&  width);


    private:
        webrtc::Mutex m_lock_video;
        webrtc::Mutex m_lock_image;
        uavos::util::CTimer m_timer_video;
        uavos::util::CTimer m_timer_image;
};


};
}; // namespace uavos

#endif