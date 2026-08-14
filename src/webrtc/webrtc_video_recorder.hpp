#ifndef VIDEORECORDER_H
#define VIDEORECORDER_H


#include <vector>
#include <string>


namespace de
{
namespace stream_webrtc
{
    
class CRecorderEvents
{
    public:

        virtual ~CRecorderEvents() {}

        // gcs_image: when non-empty, these bytes are sent to the GCS instead of
        // reading output_file_name from disk. Used to downlink a low-res still
        // while keeping the full-res file locally.
        virtual void onImageRecorded(std::string output_file_name, bool send_image_gcs, std::vector<unsigned char> gcs_image){}
        virtual void onVideoStarted(){}
        virtual void onVideoStopped(){}
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
        // gcs_small: when true, a low-res PNG (per gcs_image_small_width/height
        // config) is sent to the GCS; the locally saved file stays full-res.
        bool takeImage(const uint &image_count, const uint &image_duration, bool gcs_small, de::stream_webrtc::CRecorderEvents * recorder_events);
    
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
        bool saveImageinPNG() const;
        bool sendImageToGCS() const;
        int getVideoRecordingBitrateKbps() const;
        int getVideoRecordingFps() const;
        bool useHardwareVideoEncoder() const;
        std::string buildFfmpegRecordCommand(const int& width, const int& height) const;
        // Builds a downscaled in-memory PNG for GCS downlink based on
        // gcs_image_small_width/height config. Returns empty when unlimited (0/-1)
        // or on failure, in which case the caller falls back to the saved file.
        // Static: uses only the singleton config + the refcounted frame buffer,
        // so it is safe to call from a detached thread without `this`.
        static std::vector<unsigned char> buildSmallGCSPng(const webrtc::VideoFrame& frame);

    private:
        uint m_frame_duration = 100;
        uint m_fps = 10;
        // true once the ffmpeg encoder process has been spawned for the current recording
        bool m_video_pipe_started = false;
        // when true, a low-res PNG is sent to GCS instead of the full-res saved file
        bool m_gcs_small = false;
        webrtc::Mutex m_lock_video;
        webrtc::Mutex m_lock_image;
        de::util::CTimer m_timer_video;
        de::util::CTimer m_timer_image;

        de::stream_webrtc::CRecorderEvents * m_recorder_events = nullptr;
};


}
} // namespace de

#endif