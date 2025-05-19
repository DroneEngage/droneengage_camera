#include "../common.h"


using namespace de;
using namespace de::stream_webrtc;


/**
 * Statuic function: creates Capturer and inistantiate instance of CapturerTrackSource.
 */
webrtc::scoped_refptr<CapturerTrackSource> de::stream_webrtc::CapturerTrackSource::Create(VideoDevCapturerComposite* videoDevCapturerComposite)
{
  if (!videoDevCapturerComposite)
  {
    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "BAD  VideoDevCapturerComposite" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    return nullptr;
  }

  std::shared_ptr<VideoDevCapturerComposite> capturer(videoDevCapturerComposite);
  return webrtc::make_ref_counted<CapturerTrackSource>(capturer);
}

