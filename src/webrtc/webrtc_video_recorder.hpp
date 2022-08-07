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
        bool takeImage(const uint &image_count, const uint &image_duration);
    
    protected:
        int printPlane(const uint8_t* buf,
               const int& width,
               const int& height,
               const int& stride);
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
        const std::string getMediaFolderPath() const;


    private:
        uint m_frame_duration = 100;
        uint m_fps = 10; 
        bool m_video_file_header_written = false;
        webrtc::Mutex m_lock_video;
        webrtc::Mutex m_lock_image;
        uavos::util::CTimer m_timer_video;
        uavos::util::CTimer m_timer_image;
};


};
}; // namespace uavos

#endif