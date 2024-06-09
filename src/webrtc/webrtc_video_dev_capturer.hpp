
#ifndef VIDEOCAPTURER_H
#define VIDEOCAPTURER_H
#include "webrtc_video_recorder.hpp"


namespace de
{
namespace stream_webrtc
{
    
// This class implements rtc::VideoSourceInterface and rtc::VideoSinkInterface
// rtc::VideoSinkInterface recieves onFrame call while rtc::VideoSinkInterface
// is used to borad cast frame data using rtc::VideoBroadcaster.
class VideoDevCapturerComposite : public rtc::VideoSourceInterface<webrtc::VideoFrame> 
                                , public rtc::VideoSinkInterface  <webrtc::VideoFrame>
                                , public de::stream_webrtc::CVideoRecording
{
    public:
        // Class used to process video frame.
        class FramePreprocessorBase {
            public:
            virtual ~FramePreprocessorBase() = default;

            virtual webrtc::VideoFrame Preprocess(const webrtc::VideoFrame& frame) = 0;
        };

    public:
        // Static function that creates VideoDevCapturerComposite 
        // internally a Capturer is created and linked to Sink
        static VideoDevCapturerComposite* Create(size_t width,
                             size_t height,
                             size_t target_fps,
                             const char * unique_name);
  
        ~VideoDevCapturerComposite() override;


    

    public:
        bool StartCapture();
        bool StopCapture();
        bool isCapturing() { 
            if (m_Capturer== nullptr) return false;
            return m_Capturer->CaptureStarted();}    
    
    public:

        void setFrameRotation (webrtc::VideoRotation rotation)
        {
            m_rotation = rotation;
        }


    public:
        // rtc::VideoSourceInterface
        void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                       const rtc::VideoSinkWants& wants) override;
        void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override;
        
        // take instance of FrameProcessor used to process video frame.
        void SetFramePreprocessor(std::unique_ptr<FramePreprocessorBase> preprocessor) {
            webrtc::MutexLock lock(&lock_);
            m_preprocessor = std::move(preprocessor);
        }
        
    
    protected:
        rtc::VideoSinkWants GetSinkWants();


    protected:
        // rtc::VideoSinkInterface
        void OnFrame(const webrtc::VideoFrame& frame) override;
        //void OnDiscardedFrame() override;
    
    private:
        void UpdateVideoAdapter();
        webrtc::VideoFrame MaybePreprocess(const webrtc::VideoFrame& frame);

        webrtc::Mutex lock_;
        // link to frame processor. you can use it to pre-process video.
        std::unique_ptr<FramePreprocessorBase> m_preprocessor RTC_GUARDED_BY(lock_);
        
        // VideoBroadcaster broadcast video frames to sinks and combines VideoSinkWants
        // from its sinks.
        rtc::VideoBroadcaster m_broadCaster;
        
        cricket::VideoAdapter m_videoAdapter;
        rtc::scoped_refptr<webrtc::VideoCaptureModule> m_Capturer;
        webrtc::VideoCaptureCapability m_cabability;
        
        // private constructor
        VideoDevCapturerComposite() : m_Capturer(nullptr) {}

        // create capturer and attach it to VideoDevCapturerComposite
        bool Init(size_t width, size_t height, size_t target_fps, const char * unique_name);
        void Destroy();

        bool m_once = false;

        webrtc::VideoRotation m_rotation;
        int8_t counter = 0;
        FILE* m_output_file;
};

}
} // namespace de

#endif