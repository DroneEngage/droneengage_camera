
#ifndef VIDEOCAPTURER_H
#define VIDEOCAPTURER_H


#include "webrtc_video_recorder.hpp"


namespace de
{
namespace stream_webrtc
{
    
// This class implements webrtc::VideoSourceInterface and webrtc::VideoSinkInterface
// webrtc::VideoSinkInterface recieves onFrame call while webrtc::VideoSinkInterface
// is used to borad cast frame data using webrtc::VideoBroadcaster.
class VideoDevCapturerComposite : public webrtc::VideoSourceInterface<webrtc::VideoFrame> 
                                , public webrtc::VideoSinkInterface  <webrtc::VideoFrame>
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
            if (m_Capturer== nullptr) 
            {
                #ifdef DDEBUG
                std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
                #endif
                return false;
            }
            // #ifdef DDEBUG
            //     std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
            //     std::cout << "Capture started" << m_Capturer->CaptureStarted() << std::endl;
            // #endif
            // return m_Capturer->CaptureStarted();

            return m_active;
        }
    
    public:

        void setFrameRotation (webrtc::VideoRotation rotation)
        {
            m_rotation = rotation;
        }


    public:
        // webrtc::VideoSourceInterface
        void AddOrUpdateSink(webrtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                       const webrtc::VideoSinkWants& wants) override;
        void RemoveSink(webrtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override;
        
        // take instance of FrameProcessor used to process video frame.
        void SetFramePreprocessor(std::unique_ptr<FramePreprocessorBase> preprocessor) {
            webrtc::MutexLock lock(&lock_);
            m_preprocessor = std::move(preprocessor);
        }
        
    
    protected:
        webrtc::VideoSinkWants GetSinkWants();


    protected:
        // webrtc::VideoSinkInterface
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
        webrtc::VideoBroadcaster m_broadCaster;
        
        webrtc::VideoAdapter m_videoAdapter;
        webrtc::scoped_refptr<webrtc::VideoCaptureModule> m_Capturer;
        webrtc::VideoCaptureCapability m_cabability;
        
        // private constructor
        VideoDevCapturerComposite() : m_Capturer(nullptr) {}

        // create capturer and attach it to VideoDevCapturerComposite
        bool Init(size_t width, size_t height, size_t target_fps, const char * unique_name);
        void Destroy();

        bool m_once = false;

        webrtc::VideoRotation m_rotation;
        

        bool m_active = false;
};

}
} // namespace de

#endif