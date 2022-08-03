

#ifndef VIDEORECORDER_H
#define VIDEORECORDER_H


namespace uavos
{
namespace stream_webrtc
{
    

class  CVideoRecording 
{
        
    public:

        bool startRecording(const std::string& file_name);
        bool isRecording();
        bool stopRecording();
        bool screenShot(const std::string file_name, const uint &image_cound, const uint &image_duration);
    
    protected:
        int printPlane(const uint8_t* buf,
               int width,
               int height,
               int stride);
        int printVideoFrame(const webrtc::VideoFrame& frame);
        int saveFrameAsRGB(webrtc::VideoFrame& frame);


    protected:
        bool m_is_video_recording = false;
        uint m_image_count = 2;
        uint m_image_duration;
        std::string m_image_file_name;
        std::string m_video_file_name;
        FILE *m_video_handler = nullptr;

    private:
        unsigned char* createBitmapFileHeader (int height, int stride);
        unsigned char* createBitmapInfoHeader (int height, int width);


    private:
        webrtc::Mutex lock_;
        
};


};
}; // namespace uavos

#endif