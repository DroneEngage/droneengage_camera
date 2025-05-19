#include "../common.h"





// as a static member it should be initialized here.
webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> de::stream_webrtc::CUserMedia::m_peerConnectionFactory = nullptr;
std::unique_ptr<webrtc::Thread> de::stream_webrtc::CUserMedia::g_worker_thread = nullptr;
std::unique_ptr<webrtc::Thread> de::stream_webrtc::CUserMedia::g_signaling_thread = nullptr;
std::unique_ptr<webrtc::Thread> de::stream_webrtc::CUserMedia::g_networking_thread = nullptr;

de::stream_webrtc::CUserMedia::CUserMedia() 
{
  m_peerCount =0;
}


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


void de::stream_webrtc::CUserMedia::DeletePeerConnection ()
{
  #ifdef DDEBUG
    std::cout << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

  de::stream_webrtc::CUserMedia::m_peerConnectionFactory = nullptr;

  #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

}


bool de::stream_webrtc::CUserMedia::InitializePeerConnection() {
  
  #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

  std::cout << "CreateLocalMediaStream Call" << std::endl;
  if (de::stream_webrtc::CUserMedia::m_peerConnectionFactory.get() == nullptr)
  {
    //https://groups.google.com/forum/#!topic/discuss-webrtc/oWYy9JwK56M
    // without creating threads and starting the signal thread messaging will not work
    // and OnSuccess is not called when creating an offer.
    //webrtc::Thread::Current()->Start();
    de::stream_webrtc::CUserMedia::g_worker_thread = webrtc::Thread::Create();
    de::stream_webrtc::CUserMedia::g_worker_thread->SetName("webrtc_worker", nullptr);
    de::stream_webrtc::CUserMedia::g_worker_thread->Start();
    de::stream_webrtc::CUserMedia::g_signaling_thread = webrtc::Thread::Create();
    de::stream_webrtc::CUserMedia::g_signaling_thread->SetName("webrtc_signaling", nullptr);
    de::stream_webrtc::CUserMedia::g_signaling_thread->Start();
    de::stream_webrtc::CUserMedia::g_networking_thread = webrtc::Thread::CreateWithSocketServer();
    de::stream_webrtc::CUserMedia::g_networking_thread->SetName("webrtc_networking", nullptr);
    de::stream_webrtc::CUserMedia::g_networking_thread->Start();

    


    

  std::unique_ptr< webrtc::VideoEncoderFactory> factory = std::make_unique<de::stream_webrtc::CBuiltinVideoEncoderFactory>();  
  
  //!TODO IMPPORTANT REVIEW CreatePeerConnectionFactory FUNCTION in webrtc/src/pc/test/peer_connection_test_wrapper.cc
  
  de::stream_webrtc::CUserMedia::m_peerConnectionFactory = 
            webrtc::CreatePeerConnectionFactory(
                de::stream_webrtc::CUserMedia::g_networking_thread.get(), 
                de::stream_webrtc::CUserMedia::g_worker_thread.get(),
                de::stream_webrtc::CUserMedia::g_signaling_thread.get(),

                webrtc::scoped_refptr<webrtc::AudioDeviceModule>(CFakeAudioCaptureModule::Create()),
                webrtc::CreateBuiltinAudioEncoderFactory(),
                webrtc::CreateBuiltinAudioDecoderFactory(),
                std::move (factory),
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

  // TODO:  change m_videoTracks type to accept : m_videoTracks.insert(std::make_pair(trackLabel,videoTrackInterface));
  m_videoTracks.insert(std::make_pair(trackLabel, videoTrackInterface.get()));


  std::cout << __FULL_DEBUG__   <<  _LOG_CONSOLE_BOLD_TEXT << "DEBUG: CreateVideoTrackInterface " << _NORMAL_CONSOLE_TEXT_ << std::endl;

  #ifdef DDEBUG
  std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif
  
  return videoTrackInterface;
}






