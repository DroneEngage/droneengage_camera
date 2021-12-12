#ifndef CVideoSink_H
#define CVideoSink_H


namespace uavos
{
namespace stream_webrtc
{
class  CVideoSink : public rtc::VideoSinkInterface<webrtc::VideoFrame>
{

    public:
    
        CVideoSink() {}
        ~CVideoSink() 
        {
            std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << "Key " << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: ~CVideoSink::CVideoSink" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        }

        void OnFrame(const webrtc::VideoFrame& frame) override;

        void OnDiscardedFrame() override;


};

}
}
#endif