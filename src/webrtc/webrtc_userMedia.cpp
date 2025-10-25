#include "../common.h"





// as a static member it should be initialized here.
webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> de::stream_webrtc::CUserMedia::m_peerConnectionFactory = nullptr;
std::unique_ptr<webrtc::Thread> de::stream_webrtc::CUserMedia::g_worker_thread = nullptr;
std::unique_ptr<webrtc::Thread> de::stream_webrtc::CUserMedia::g_signaling_thread = nullptr;
std::unique_ptr<webrtc::Thread> de::stream_webrtc::CUserMedia::g_networking_thread = nullptr;

// Class responsible for managing WebRTC user media operations, including initialization and stream handling.
de::stream_webrtc::CUserMedia::CUserMedia() 
{
  m_peerCount =0;
}

// Destructor ensures proper cleanup of WebRTC resources.
de::stream_webrtc::CUserMedia::~CUserMedia()
{
  #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

  DeletePeerConnection();



  #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

}

// Cleans up WebRTC resources by releasing the PeerConnectionFactory and stopping threads.
void de::stream_webrtc::CUserMedia::DeletePeerConnection ()
{
  #ifdef DDEBUG
    std::cout << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

  // 1. Release the PeerConnectionFactory.
  // This will decrement its reference count. If no other scoped_refptr
  // holds a reference, the factory object will be destroyed.
  de::stream_webrtc::CUserMedia::m_peerConnectionFactory = nullptr;

  // 2. Stop and reset the WebRTC threads.
  // It's crucial to stop them gracefully before resetting the unique_ptr,
  // as stopping might involve waiting for pending tasks to complete on those threads.
  // The order of stopping (networking, worker, signaling) is generally recommended
  // as networking thread often relies on worker/signaling for callbacks.
  if (de::stream_webrtc::CUserMedia::g_networking_thread) {
    #ifdef DDEBUG
      std::cout << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " Stopping networking thread." << std::endl;
    #endif
    de::stream_webrtc::CUserMedia::g_networking_thread->Stop();
    de::stream_webrtc::CUserMedia::g_networking_thread.reset(); // Release unique_ptr ownership
  }

  if (de::stream_webrtc::CUserMedia::g_worker_thread) {
    #ifdef DDEBUG
      std::cout << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " Stopping worker thread." << std::endl;
    #endif
    de::stream_webrtc::CUserMedia::g_worker_thread->Stop();
    de::stream_webrtc::CUserMedia::g_worker_thread.reset(); // Release unique_ptr ownership
  }

  if (de::stream_webrtc::CUserMedia::g_signaling_thread) {
    #ifdef DDEBUG
      std::cout << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " Stopping signaling thread." << std::endl;
    #endif
    de::stream_webrtc::CUserMedia::g_signaling_thread->Stop();
    de::stream_webrtc::CUserMedia::g_signaling_thread.reset(); // Release unique_ptr ownership
  }





  #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

}

// Initializes WebRTC environment by setting up threads and creating PeerConnectionFactory.
bool de::stream_webrtc::CUserMedia::InitializePeerConnection() {
  
  #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

  std::cout << "Attempting to initialize PeerConnectionFactory..." << std::endl;
  
  // Only create the PeerConnectionFactory and threads if they are not already initialized.
  if (de::stream_webrtc::CUserMedia::m_peerConnectionFactory.get() == nullptr)
  {
    //https://groups.google.com/forum/#!topic/discuss-webrtc/oWYy9JwK56M
    // without creating threads and starting the signal thread messaging will not work
    // and OnSuccess is not called when creating an offer.
    
    // Before creating new threads, ensure no old threads are lingering.
    // This check is important if InitializePeerConnection might be called
    // without a prior, complete DeletePeerConnection.
    if (de::stream_webrtc::CUserMedia::g_networking_thread ||
        de::stream_webrtc::CUserMedia::g_worker_thread ||
        de::stream_webrtc::CUserMedia::g_signaling_thread)
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Warning: WebRTC threads are already active but PeerConnectionFactory is null. Attempting full cleanup before re-initialization." << _NORMAL_CONSOLE_TEXT_ << std::endl;
        DeletePeerConnection(); // Perform a full cleanup if threads are unexpectedly active
    }

    // Create and start the WebRTC threads.
    // These threads are essential for WebRTC's internal operations.
    de::stream_webrtc::CUserMedia::g_worker_thread = webrtc::Thread::Create();
    de::stream_webrtc::CUserMedia::g_worker_thread->SetName("webrtc_worker", nullptr);
    de::stream_webrtc::CUserMedia::g_worker_thread->Start();

    de::stream_webrtc::CUserMedia::g_signaling_thread = webrtc::Thread::Create();
    de::stream_webrtc::CUserMedia::g_signaling_thread->SetName("webrtc_signaling", nullptr);
    de::stream_webrtc::CUserMedia::g_signaling_thread->Start();

    de::stream_webrtc::CUserMedia::g_networking_thread = webrtc::Thread::CreateWithSocketServer();
    de::stream_webrtc::CUserMedia::g_networking_thread->SetName("webrtc_networking", nullptr);
    de::stream_webrtc::CUserMedia::g_networking_thread->Start();

    // Create the video encoder factory.
    std::unique_ptr< webrtc::VideoEncoderFactory> videoEncoderFactory = std::make_unique<de::stream_webrtc::CBuiltinVideoEncoderFactory>();

  //!TODO IMPPORTANT REVIEW CreatePeerConnectionFactory FUNCTION in webrtc/src/pc/test/peer_connection_test_wrapper.cc
  
    de::stream_webrtc::CUserMedia::m_peerConnectionFactory = 
            webrtc::CreatePeerConnectionFactory(
                de::stream_webrtc::CUserMedia::g_networking_thread.get(), 
                de::stream_webrtc::CUserMedia::g_worker_thread.get(),
                de::stream_webrtc::CUserMedia::g_signaling_thread.get(),

                webrtc::scoped_refptr<webrtc::AudioDeviceModule>(CFakeAudioCaptureModule::Create()),
                webrtc::CreateBuiltinAudioEncoderFactory(),
                webrtc::CreateBuiltinAudioDecoderFactory(),
                std::move (videoEncoderFactory),
                std::make_unique<webrtc::VideoDecoderFactoryTemplate<
                      webrtc::LibvpxVp8DecoderTemplateAdapter,
                      webrtc::LibvpxVp9DecoderTemplateAdapter,
                      webrtc::OpenH264DecoderTemplateAdapter,
                      webrtc::Dav1dDecoderTemplateAdapter>>(), 
                nullptr, /* audio_mixer */
                nullptr, /* audio_processing */
                nullptr,
                nullptr);
                
  
  #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << ".1" << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

  } 

  if (!de::stream_webrtc::CUserMedia::m_peerConnectionFactory)
  {
    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Internal Error: CreateLocalMediaStream failed" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    DeletePeerConnection();
    return false;
  }  
  
  std::cout << "CreatePeerConnectionFactory Created" << std::endl;
  
  #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << ".2" << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif
  
  return true;
}

// Creates a local media stream with the specified stream ID.
bool de::stream_webrtc::CUserMedia::CreateLocalMediaStream(const char * streamId) {
  
  #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif
  
  if (m_peerConnectionFactory.get()== nullptr)
  {
    return false;
  }
  
  m_stream  = m_peerConnectionFactory->CreateLocalMediaStream(streamId);
  std::cout << _SUCCESS_CONSOLE_TEXT_ << "stream created :" << streamId << _NORMAL_CONSOLE_TEXT_ << std::endl;

  #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif
  
  return true;
}

// Removes all video tracks from the current stream.
bool de::stream_webrtc::CUserMedia::RemoveVideoTracks ()
{
  #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif
  
  webrtc::VideoTrackVector videoTracks = m_stream.get()->GetVideoTracks(); 

    if (videoTracks.empty())
    return true;

    for (auto& track : videoTracks) {
      m_stream.get()->RemoveTrack (track);
    }

  #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif
  
  
    
    return true;
}

// Removes all audio tracks from the current stream.
bool de::stream_webrtc::CUserMedia::RemoveAudioTracks ()
{
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  
    webrtc::AudioTrackVector audioTracks = m_stream.get()->GetAudioTracks(); 

    if (audioTracks.empty())
    return true;

    for (auto& track : audioTracks) {
      m_stream.get()->RemoveTrack (track);
    }
    
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
  
    return true;
}

// Creates a video track interface with the given label and source.
webrtc::scoped_refptr<webrtc::VideoTrackInterface> de::stream_webrtc::CUserMedia::CreateVideoTrackInterface (const std::string& trackLabel, 
            webrtc::VideoTrackSourceInterface* videoTrackSourceInterface
        )
{
  #ifdef DDEBUG
  std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif
  
  webrtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrackInterface (m_peerConnectionFactory.get()->CreateVideoTrack(trackLabel, videoTrackSourceInterface));
  if ((videoTrackInterface == nullptr) || (!videoTrackInterface.get()))
  {
    std::cout << __FULL_DEBUG__   << _ERROR_CONSOLE_BOLD_TEXT_ << "could not create CreateVideoTrack"  << trackLabel << _NORMAL_CONSOLE_TEXT_ << std::endl;
    return nullptr;
  }


  std::cout << __FULL_DEBUG__   <<  _LOG_CONSOLE_BOLD_TEXT << "DEBUG: CreateVideoTrackInterface " << _NORMAL_CONSOLE_TEXT_ << std::endl;

  #ifdef DDEBUG
  std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif
  
  return videoTrackInterface;
}
