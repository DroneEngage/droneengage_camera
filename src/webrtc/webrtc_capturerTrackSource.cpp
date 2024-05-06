#include "../common.h"


using namespace uavos;
using namespace uavos::stream_webrtc;


/**
 * Statuic function: creates Capturer and inistantiate instance of CapturerTrackSource.
 */
rtc::scoped_refptr<CapturerTrackSource>   uavos::stream_webrtc::CapturerTrackSource::Create(VideoDevCapturerComposite*  videoDevCapturerComposite) 
{

  if (!videoDevCapturerComposite) 
  {
    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "BAD  VideoDevCapturerComposit" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    return nullptr;
  }

  std::shared_ptr<VideoDevCapturerComposite> capturer (videoDevCapturerComposite);
  // attach to webrtc::VideoTrackSource and create CapturerTrackSource instance.
  return new rtc::RefCountedObject<CapturerTrackSource>(capturer);
}

