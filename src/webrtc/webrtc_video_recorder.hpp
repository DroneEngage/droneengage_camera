#ifndef VIDEORECORDER_H
#define VIDEORECORDER_H


namespace de
{
namespace stream_webrtc
{
    
class CRecorderEvents
{
    public:
        virtual void onImageRecorded(std::string output_file_name, bool send_image_gcs){};
        virtual void onVideoStarted(){};
        virtual void onVideoStopped(){};
};

class  CVideoRecording 
{
        
    public:
        void RegisterRecorderEvents (de::stream_webrtc::CRecorderEvents * recorder_events) 
        {
            m_recorder_events = recorder_events;
        }

    public:
        bool startRecording();
        bool isRecording();
        bool stopRecording();
        bool takeImage(const uint &image_count, const uint &image_duration, de::stream_webrtc::CRecorderEvents * recorder_events);
    
    protected:
        int printPlane(const uint8_t* buf,
               const int& width,
               const int& height,
               const int& stride);
        int printVideoFrame(const webrtc::VideoFrame& frame);
        int saveFrameAsRGB(webrtc::VideoFrame& frame);
        int saveFrameAsJPG(webrtc::VideoFrame& frame);
        int saveFrameAsPNG(webrtc::VideoFrame& frame);


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
        const bool saveImageinPNG() const;
        const bool sendImageToGCS() const;

    private:
        uint m_frame_duration = 100;
        uint m_fps = 10; 
        bool m_video_file_header_written = false;
        webrtc::Mutex m_lock_video;
        webrtc::Mutex m_lock_image;
        de::util::CTimer m_timer_video;
        de::util::CTimer m_timer_image;

        de::stream_webrtc::CRecorderEvents * m_recorder_events = nullptr;
};


}
} // namespace de

#endif