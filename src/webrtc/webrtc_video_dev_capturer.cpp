#include "../common.h"
using namespace de;
using namespace de::stream_webrtc;

#include <chrono> // For high-resolution time


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
    
    // Preprocessing the frame: apply any necessary transformations early.
    webrtc::VideoFrame processed_frame = MaybePreprocess(original_frame);


    // --- Frame Rate Calculation Start ---
    // Initialize on the first call
    if (m_last_frame_rate_check_time.time_since_epoch().count() == 0) {
        m_last_frame_rate_check_time = std::chrono::high_resolution_clock::now();
        m_frame_count_for_fps = 0;
    }

    m_frame_count_for_fps++;

    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_frame_rate_check_time);

    if (elapsed_time.count() >= m_fps_interval_ms) {
        m_current_frame_rate = static_cast<double>(m_frame_count_for_fps) / (elapsed_time.count() / 1000.0);
        std::cout << _LOG_CONSOLE_BOLD_TEXT << "Current Frame Rate: " << _INFO_BOLD_CONSOLE_TEXT << m_current_frame_rate << _LOG_CONSOLE_BOLD_TEXT << " FPS" << _NORMAL_CONSOLE_TEXT_ << std::endl;

        // Reset for the next interval
        m_frame_count_for_fps = 0;
        m_last_frame_rate_check_time = now;
    }
    // --- Frame Rate Calculation End ---

    // Handle frame persistence (saving video/images)
    // These operations should ideally be offloaded to avoid blocking the video pipeline.
    // Consider using a separate thread or an asynchronous queue for these.
    if (m_is_video_recording) {
        printVideoFrame(processed_frame);
    }
    saveFrameAsPNG(processed_frame);
    saveFrameAsRGB(processed_frame);

    // Initial log for function entry (consider removing after debugging)
    // This `m_once` flag logic is usually for debugging and can be removed in production.
    if (!m_once) {
      #ifdef DEBUG
        std::cout << "VideoDevCapturerComposite::onFrame - First call" << std::endl;
      #endif 
        m_once = true;
    }

    // Define target dimensions for scaling.
    // It's generally better to make these configurable or derived from system capabilities.
    int target_out_width = 1280;
    int target_out_height = 780;

    // Check if scaling is necessary based on target dimensions.
    // If the frame is already smaller than or equal to the target, broadcast directly.
    if (processed_frame.width() <= target_out_width && processed_frame.height() <= target_out_height) {
        m_broadCaster.OnFrame(processed_frame);
        return; // No scaling needed, return early.
    }

    int adapted_width = 0;
    int adapted_height = 0;

    // Adapt frame resolution using the video adapter.
    // The adapter will determine the 'adapted_width' and 'adapted_height'
    // based on internal logic, network conditions, and sink wants.
    // The previous 'target_out_width' and 'target_out_height' variables were redundant here
    // as the adapter's primary output resolution is driven by other factors (like OnSinkWants).
    if (!m_videoAdapter.AdaptFrameResolution(
            processed_frame.width(), processed_frame.height(), processed_frame.timestamp_us() * 1000,
            &adapted_width, &adapted_height,
            (int*)&target_out_width, (int*)&target_out_height)) {
        #ifdef DEBUG
        // Log frame drop for debugging.
        std::cout << "DEBUG: Frame dropped to respect frame rate constraint." << std::endl;
        #endif
        return; // Frame dropped, return early.
    }

    // If adaptation results in different dimensions, scale the frame.
    // This check is crucial: it determines if scaling is needed at all.
    if (adapted_height != processed_frame.height() || adapted_width != processed_frame.width()) {
        // Allocate a new buffer and scale the frame.
        webrtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer =
            webrtc::I420Buffer::Create(adapted_width, adapted_height);
        scaled_buffer->ScaleFrom(*processed_frame.video_frame_buffer()->ToI420());

        // Build the new scaled video frame.
        webrtc::VideoFrame::Builder new_frame_builder = webrtc::VideoFrame::Builder()
            .set_video_frame_buffer(scaled_buffer)
            .set_rotation(webrtc::kVideoRotation_0)
            .set_timestamp_us(processed_frame.timestamp_us())
            .set_id(processed_frame.id());

        webrtc::VideoFrame adapted_frame = new_frame_builder.build();

        // If the original frame had an update rectangle, scale it.
        if (processed_frame.has_update_rect()) {
            webrtc::VideoFrame::UpdateRect new_rect = processed_frame.update_rect().ScaleWithFrame(
                processed_frame.width(), processed_frame.height(), 0, 0,
                processed_frame.width(), processed_frame.height(),
                adapted_width, adapted_height);
            adapted_frame.set_update_rect(new_rect);
        }

        m_broadCaster.OnFrame(adapted_frame); // Broadcast the scaled frame.

        #ifdef DEBUG
        std::cout << __FUNCTION__ << ":" << __LINE__ << " DEBUG: Video adapter requested down-scale. Broadcasting scaled version." << std::endl;
        #endif

    } else {
        // If no scaling was requested by the adapter, broadcast the original processed frame.
        m_broadCaster.OnFrame(processed_frame);
    }

    // Debug logging at function exit
    #ifdef D3DEBUG
    std::cout << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
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




