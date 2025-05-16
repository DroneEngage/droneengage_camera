#include "../common.h"

#include "webrtc_userMedia.hpp"

#include "webrtc_fakeAudioCaptureModule.hpp"


// as a static member it should be initialized here.
rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> de::stream_webrtc::CUserMedia::m_peerConnectionFactory = nullptr;
std::unique_ptr<rtc::Thread> de::stream_webrtc::CUserMedia::g_worker_thread = nullptr;
std::unique_ptr<rtc::Thread> de::stream_webrtc::CUserMedia::g_signaling_thread = nullptr;
std::unique_ptr<rtc::Thread> de::stream_webrtc::CUserMedia::g_networking_thread = nullptr;
rtc::Thread* de::stream_webrtc::CUserMedia::thread = nullptr;

de::stream_webrtc::CUserMedia::CUserMedia() 
{
  m_peerCount =0;
}


de::stream_webrtc::CUserMedia::~CUserMedia()
{
  std::cout << __FULL_DEBUG__   << " " << "Key " << "\033[1;31m" << "DESTRUCTOR" << _NORMAL_CONSOLE_TEXT_ << std::endl;
  
  DeletePeerConnection();
}


void de::stream_webrtc::CUserMedia::DeletePeerConnection ()
{
  std::cout << __FULL_DEBUG__   << " " << "Key " << "\033[1;31m" << "DESTRUCTOR" << _NORMAL_CONSOLE_TEXT_ << std::endl;
  
  de::stream_webrtc::CUserMedia::m_peerConnectionFactory = nullptr;
}


bool de::stream_webrtc::CUserMedia::InitializePeerConnection() {
  
  std::cout << "CreateLocalMediaStream Call" << std::endl;
  if (de::stream_webrtc::CUserMedia::m_peerConnectionFactory.get() == nullptr)
  {
    //https://groups.google.com/forum/#!topic/discuss-webrtc/oWYy9JwK56M
    // without creating threads and starting the signal thread messaging will not work
    // and OnSuccess is not called when creating an offer.
    //rtc::Thread::Current()->Start();
    de::stream_webrtc::CUserMedia::g_worker_thread = rtc::Thread::Create();
    de::stream_webrtc::CUserMedia::g_worker_thread->Start();
    de::stream_webrtc::CUserMedia::g_signaling_thread = rtc::Thread::Create();
    de::stream_webrtc::CUserMedia::g_signaling_thread->Start();
    de::stream_webrtc::CUserMedia::g_networking_thread = rtc::Thread::CreateWithSocketServer();
    de::stream_webrtc::CUserMedia::g_networking_thread->Start();

    //rtc::ThreadManager::Instance()->WrapCurrentThread();


    

    de::stream_webrtc::CUserMedia::thread = rtc::ThreadManager::Instance()->WrapCurrentThread();
  

  std::unique_ptr< webrtc::VideoEncoderFactory> factory = std::make_unique<de::stream_webrtc::CBuiltinVideoEncoderFactory>();  //webrtc::CreateBuiltinVideoEncoderFactory();
  //std::unique_ptr< webrtc::VideoEncoderFactory> factory = webrtc::CreateBuiltinVideoEncoderFactory();  

  // Printed in std::cout so no need to list it.
  std::vector<webrtc::SdpVideoFormat> format = factory->GetSupportedFormats();

  
  de::stream_webrtc::CUserMedia::m_peerConnectionFactory = rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> (
            webrtc::CreatePeerConnectionFactory(
                de::stream_webrtc::CUserMedia::g_networking_thread.get(), 
                de::stream_webrtc::CUserMedia::g_worker_thread.get(),
                de::stream_webrtc::CUserMedia::g_signaling_thread.get(),

                rtc::scoped_refptr<webrtc::AudioDeviceModule>(CFakeAudioCaptureModule::Create()),
                // nullptr /* network_thread */, nullptr /* worker_thread */,
                // nullptr /* signaling_thread */, nullptr /* default_adm */,
                webrtc::CreateBuiltinAudioEncoderFactory(),
                webrtc::CreateBuiltinAudioDecoderFactory(),
                std::move (factory),
                webrtc::CreateBuiltinVideoDecoderFactory(), 
                nullptr /* audio_mixer */,
                nullptr /* audio_processing */).get());
  } 

  if (!de::stream_webrtc::CUserMedia::m_peerConnectionFactory.get())
  {
    // try again
    std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Internal Error: CreateLocalMediaStream failed" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    DeletePeerConnection();
    return false;
  }  
  
  std::cout << "CreatePeerConnectionFactory Created" << std::endl;
  
  return true;
}

bool de::stream_webrtc::CUserMedia::CreateLocalMediaStream(const char * streamId) {
  if (m_peerConnectionFactory.get()== nullptr)
  {
    return false;
  }

  m_stream = rtc::scoped_refptr<webrtc::MediaStreamInterface> (m_peerConnectionFactory.get()->CreateLocalMediaStream(streamId).get());
  std::cout << _SUCCESS_CONSOLE_TEXT_ << "stream created :" << streamId << _NORMAL_CONSOLE_TEXT_ << std::endl;

  return true;
}


bool de::stream_webrtc::CUserMedia::RemoveVideoTracks ()
{
    webrtc::VideoTrackVector videoTracks = m_stream.get()->GetVideoTracks(); 

    if (videoTracks.empty())
    return true;

    for (auto& track : videoTracks) {
      m_stream.get()->RemoveTrack (track);
    }
    
    return true;
}

bool de::stream_webrtc::CUserMedia::RemoveAudioTracks ()
{
    webrtc::AudioTrackVector audioTracks = m_stream.get()->GetAudioTracks(); 

    if (audioTracks.empty())
    return true;

    for (auto& track : audioTracks) {
      m_stream.get()->RemoveTrack (track);
    }
    
    return true;
}


rtc::scoped_refptr<webrtc::VideoTrackInterface> de::stream_webrtc::CUserMedia::CreateVideoTrackInterface (const std::string& trackLabel, 
            webrtc::VideoTrackSourceInterface* videoTrackSourceInterface
        )
{
  rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrackInterface (m_peerConnectionFactory.get()->CreateVideoTrack(trackLabel, videoTrackSourceInterface));
  if ((videoTrackInterface == nullptr) || (!videoTrackInterface.get()))
  {
    std::cout << __FULL_DEBUG__   << _ERROR_CONSOLE_BOLD_TEXT_ << "could not create CreateVideoTrack"  << trackLabel << _NORMAL_CONSOLE_TEXT_ << std::endl;
    return nullptr;
  }

  m_videoTracks.insert(std::make_pair(trackLabel,videoTrackInterface));

  std::cout << __FULL_DEBUG__   <<  _LOG_CONSOLE_BOLD_TEXT << "DEBUG: CreateVideoTrackInterface " << _NORMAL_CONSOLE_TEXT_ << std::endl;

  return videoTrackInterface;
}

bool de::stream_webrtc::CUserMedia::AddVideoTrack (webrtc::VideoTrackInterface * videoTrackInterface)
{
        
  std::cout << __FULL_DEBUG__   <<  "\033[0;32m" << "AddVideoTrack "  << _NORMAL_CONSOLE_TEXT_ << std::endl;
  
  videoTrackInterface->set_enabled(true);

  if (!m_stream.get()->AddTrack(videoTrackInterface))
  {

    std::cout << __FULL_DEBUG__  << _ERROR_CONSOLE_BOLD_TEXT_ << "could NOT AddTrack"  << _NORMAL_CONSOLE_TEXT_ << std::endl;
    return false;
  }

  return true;  

}

rtc::scoped_refptr<webrtc::MediaStreamInterface> de::stream_webrtc::CUserMedia::GetMediaStream ()
{
  return m_stream;
}

rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> de::stream_webrtc::CUserMedia::GetPeerConnectionFactory ()
{
  return m_peerConnectionFactory;
}



