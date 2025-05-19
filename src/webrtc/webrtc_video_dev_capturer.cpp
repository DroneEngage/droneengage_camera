#include "../common.h"
using namespace de;
using namespace de::stream_webrtc;



// create webrtc::VideoCaptureModule capturer.
bool de::stream_webrtc::VideoDevCapturerComposite::Init(size_t width,
                       size_t height,
                       size_t target_fps,
                       const char * unique_name) {

  #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

  
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> video_info(webrtc::VideoCaptureFactory::CreateDeviceInfo());

  //Create video capturer that is linked to video device. e.g.: /dev/video1
  m_Capturer = webrtc::VideoCaptureFactory::Create(unique_name);
  if (!m_Capturer) {

    #ifdef DDEBUG
    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    return false;
  }

  // link capturer to sink to receive onFrame
  // notice that this OnFrame has nothing to do with Stream->getTrack->AddorUpdateSinlk()
  // this is a local onFrame where you can change frame contents from the very first beginning.
  // actually THIS "RegisterCaptureDataCallback" is a mandatory step for frame and delevering it to TrackSource.
  m_Capturer->RegisterCaptureDataCallback(this);

  // start capturing with given parameters.
  //video_info->GetCapability(m_Capturer->CurrentDeviceName(), 0, m_cabability);

  webrtc::VideoCaptureCapability cabability;

  cabability.width = static_cast<int32_t>(width);
  cabability.height = static_cast<int32_t>(height);
  cabability.maxFPS = static_cast<int32_t>(target_fps);
  cabability.videoType = webrtc::VideoType::kI420; //TODO: check this !!!!!!

  video_info->GetBestMatchedCapability(m_Capturer->CurrentDeviceName(), cabability, m_cabability);

  std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Valid Video Device Found: "  
        << _SUCCESS_CONSOLE_TEXT_ << " cam_unique_name:" << _INFO_BOLD_CONSOLE_TEXT << unique_name 
        << _SUCCESS_CONSOLE_TEXT_ << " width:" << _INFO_BOLD_CONSOLE_TEXT << m_cabability.width 
        << _SUCCESS_CONSOLE_TEXT_ << " height:" << _INFO_BOLD_CONSOLE_TEXT << m_cabability.height 
        << _SUCCESS_CONSOLE_TEXT_ << " maxFPS:" << _INFO_BOLD_CONSOLE_TEXT << m_cabability.maxFPS 
        << _NORMAL_CONSOLE_TEXT_ << std::endl;
        

  #ifdef DDEBUG
  std::cout << "width:" << m_cabability.width << " height:" << m_cabability.height << std::endl;        
  #endif

  #ifdef DDEBUG
  std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

  return true;
}

bool de::stream_webrtc::VideoDevCapturerComposite::StartCapture()
{
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  
    if (!m_Capturer) {
      #ifdef DDEBUG
      std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << __FILE__ << ".1." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
      #endif
    return false;
  }

  if (m_Capturer->StartCapture(m_cabability) != 0) {
    #ifdef DDEBUG
    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << __FILE__ << ".2." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  
    Destroy();
    #ifdef DDEBUG
    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << __FILE__ << ".3." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  
    return false;
  }


  if (m_Capturer->CaptureStarted())
  {
    m_active = true;
    
  }
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    std::cout << "Capture started" << m_Capturer->CaptureStarted() << std::endl;
    #endif
  
  return true;
}

bool de::stream_webrtc::VideoDevCapturerComposite::StopCapture()
{

    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  

  if (!m_Capturer) {
    m_active = false;
    return false;
  }

  if (m_Capturer->StopCapture() != 0)
  {
    return false;
  }

  m_active = false;
  
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ <<  __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  
  return true;
}

de::stream_webrtc::VideoDevCapturerComposite*  de::stream_webrtc::VideoDevCapturerComposite::Create(size_t width,
                                 size_t height,
                                 size_t target_fps,
                                 const char * unique_name) {

    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  

  // create instance.                                   
  VideoDevCapturerComposite* capturer = new VideoDevCapturerComposite();

  
  if (!capturer->Init(width, height, target_fps, unique_name)) {
    std::cout << "Failed to create capturer(w = " << width
                        << ", h = " << height << ", fps = " << target_fps
                        << ")" << std::endl;
    #ifdef DDEBUG
    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << __FILE__ << ".1" << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  
    return nullptr;
  }
  
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  
  return capturer;
}

void de::stream_webrtc::VideoDevCapturerComposite::Destroy() {
  
  #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif
  
  if (!m_Capturer)
  {

    #ifdef DDEBUG
    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << __FILE__ << ".1" << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  
    return;
  }

  m_Capturer->StopCapture();

  
  m_Capturer->DeRegisterCaptureDataCallback();
  
  
// Release reference to VCM.
  m_Capturer = nullptr;

    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  
}

de::stream_webrtc::VideoDevCapturerComposite::~VideoDevCapturerComposite() {
  
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  
  std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << "Key " << "\033[1;31m" << "DESTRUCTOR" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
  Destroy();

    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  
}                       


void de::stream_webrtc::VideoDevCapturerComposite::OnFrame(const webrtc::VideoFrame& original_frame)
{

    #ifdef D3DEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  
  webrtc::VideoFrame frame = MaybePreprocess(original_frame);
 
  if (m_is_video_recording)
  {
    // save video if needed
    printVideoFrame (frame);
  }
  
  //try
 // {
    /* code */
    // save image if needed
    saveFrameAsPNG(frame);  
    saveFrameAsRGB(frame);
  // }
  // catch(const std::exception& e)
  // {
  //   std::cerr << e.what() << '\n';
  // }
  // catch(...)
  // {

  // }
  
  if (!m_once)
  {
    std::cout <<"VideoDevCapturerComposite::onFrame" << std::endl;
    m_once = true;
  }

  int cropped_width = 0;
  int cropped_height = 0;
  int out_width = 640;
  int out_height = 480;

  
  if ((out_width>=frame.width()) || (out_height>=frame.height()))
  {
    m_broadCaster.OnFrame(frame);
    return ;
  }

  if (!m_videoAdapter.AdaptFrameResolution(
          frame.width(), frame.height(), frame.timestamp_us() * 1000,
          &cropped_width, &cropped_height, &out_width, &out_height)) 
  {
    #ifdef DEBUG
      // Drop frame in order to respect frame rate constraint.
      std::cout << "Drop frame in order to respect frame rate constraint." << std::endl;
    #endif
    return;
  }
  
  if (out_height != frame.height() || out_width != frame.width()) {
    // Video adapter has requested a down-scale. Allocate a new buffer and
    // return scaled version.
    webrtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer =
        webrtc::I420Buffer::Create(out_width, out_height);
    scaled_buffer->ScaleFrom(*frame.video_frame_buffer()->ToI420());

    webrtc::VideoFrame::Builder new_frame_builder = webrtc::VideoFrame::Builder()
            .set_video_frame_buffer(scaled_buffer)
            .set_rotation(webrtc::kVideoRotation_0)
            .set_timestamp_us(frame.timestamp_us())
            .set_id(frame.id());
    
    webrtc::VideoFrame adapted_frame = new_frame_builder.build();

    if (frame.has_update_rect()) {
      webrtc::VideoFrame::UpdateRect new_rect = frame.update_rect().ScaleWithFrame(
          frame.width(), frame.height(), 0, 0, frame.width(), frame.height(),
          out_width, out_height);
      adapted_frame.set_update_rect(new_rect);
    }
    
    m_broadCaster.OnFrame(adapted_frame);
    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: Video adapter has requested a down-scale. Allocate a new buffer and return scaled version." << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
        

  } else {
    m_broadCaster.OnFrame(frame);
  }
  
    #ifdef D3DEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  
}


webrtc::VideoSinkWants de::stream_webrtc::VideoDevCapturerComposite::GetSinkWants() {
  return m_broadCaster.wants();
}

void de::stream_webrtc::VideoDevCapturerComposite::AddOrUpdateSink( webrtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                        const webrtc::VideoSinkWants& wants) 
{
  #ifdef DDEBUG
  std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif
  
  #ifdef DEBUG
  std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "\r\n" << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: VideoDevCapturerComposite::AddOrUpdateSink" << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

  m_broadCaster.AddOrUpdateSink(sink, wants);
  UpdateVideoAdapter();
    
  #ifdef DDEBUG
  std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif
}

void de::stream_webrtc::VideoDevCapturerComposite::RemoveSink(webrtc::VideoSinkInterface<webrtc::VideoFrame>* sink) {
  
  #ifdef DDEBUG
  std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

  #ifdef DEBUG
  std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "\r\n" << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: VideoDevCapturerComposite::RemoveSink" << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

  m_broadCaster.RemoveSink(sink);
  UpdateVideoAdapter();

}

void de::stream_webrtc::VideoDevCapturerComposite::UpdateVideoAdapter() {
  // webrtc::VideoSinkWants wants = m_broadCaster.wants();
  
  // m_videoAdapter.OnResolutionFramerateRequest(
  //                   wants.target_pixel_count, 
  //                   wants.max_pixel_count, 
  //                   wants.max_framerate_fps);
}

webrtc::VideoFrame de::stream_webrtc::VideoDevCapturerComposite::MaybePreprocess(const webrtc::VideoFrame& frame) {
  webrtc::MutexLock lock(&lock_);
  
  return frame;
}




