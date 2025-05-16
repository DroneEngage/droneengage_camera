#ifndef CAPTURETRACKSOURCE_H
#define CAPTURETRACKSOURCE_H

namespace de
{
namespace stream_webrtc
{


/**
 * Here we need to link VideoSinkInterface to VideoTrackSource.
 * Capturer-> VideoSinkInterface -> VideoTrackSource -> Track -> Stream
 **/
class CapturerTrackSource : public webrtc::VideoTrackSource {
 public:
 
  ~CapturerTrackSource()
  {
        std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << "Key " << "\033[1;31m" << "DESTRUCTOR" << _NORMAL_CONSOLE_TEXT_ << std::endl;
  }
 
 
 static rtc::scoped_refptr<CapturerTrackSource>   Create(VideoDevCapturerComposite*  videoDevCapturerComposite);

 
  void Start() { SetState(webrtc::MediaSourceInterface::SourceState::kLive); }

  void Stop() { SetState(webrtc::MediaSourceInterface::SourceState::kMuted); }


 protected:
 // private constructor
   explicit CapturerTrackSource(
      std::shared_ptr<VideoDevCapturerComposite> capturer)
      : webrtc::VideoTrackSource(/*remote=*/false), m_capturer(capturer) {}

  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
      return m_capturer.get();
    }

 private:
  
  std::shared_ptr<VideoDevCapturerComposite>  m_capturer;
};

}
}
#endif