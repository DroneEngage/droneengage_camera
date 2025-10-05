
#include "../common.h"


#include "../de_common/helpers/json_nlohmann.hpp"
using Json_de = nlohmann::json;


de::stream_webrtc::CPeerConnectionManager::CPeerConnectionManager ():m_callbacks(NULL)
{
}

de::stream_webrtc::CPeerConnectionManager::CPeerConnectionManager (CCallbacks *callbacks):m_callbacks(callbacks)
{
}


de::stream_webrtc::CPeerConnectionManager::~CPeerConnectionManager()
{
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " "  << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    // IMPORTANT: Explicitly close the PeerConnection and nullify the smart pointer.
    // This ensures graceful shutdown of WebRTC's internal state and callbacks,
    // preventing use-after-move or use-after-free issues.
    if (m_peerConnection) {
        std::cout << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << _LOG_CONSOLE_BOLD_TEXT << " DEBUG: Calling m_peerConnection->Close()" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        m_peerConnection->Close();
        m_peerConnection = nullptr; // Explicitly release the reference
    }
}

void de::stream_webrtc::CPeerConnectionManager::CreateSomeMediaDeps(webrtc::PeerConnectionFactoryDependencies& media_deps) {
// CHECK /home/mhefny/TDisk_6T/Work/webrtc_2025/webrtc/src/webrtc_lib_link_test.cc
//   media_deps.adm =
//       CreateAudioDeviceModule(*media_deps.env, AudioDeviceModule::kDummyAudio);
//   media_deps.audio_encoder_factory =
//       webrtc::CreateAudioEncoderFactory<webrtc::AudioEncoderOpus>();
//   media_deps.audio_decoder_factory =
//       webrtc::CreateAudioDecoderFactory<webrtc::AudioDecoderOpus>();
//   media_deps.video_encoder_factory =
//       std::make_unique<VideoEncoderFactoryTemplate<
//           LibvpxVp8EncoderTemplateAdapter, LibvpxVp9EncoderTemplateAdapter,
//           OpenH264EncoderTemplateAdapter, LibaomAv1EncoderTemplateAdapter>>();
//   media_deps.video_decoder_factory =
//       std::make_unique<VideoDecoderFactoryTemplate<
//           LibvpxVp8DecoderTemplateAdapter, LibvpxVp9DecoderTemplateAdapter,
//           OpenH264DecoderTemplateAdapter, Dav1dDecoderTemplateAdapter>>();
//   media_deps.audio_processing_builder =
//       std::make_unique<BuiltinAudioProcessingBuilder>();
}

webrtc::PeerConnectionFactoryDependencies de::stream_webrtc::CPeerConnectionManager::CreateSomePcfDeps() {
  webrtc::PeerConnectionFactoryDependencies pcf_deps;
  pcf_deps.signaling_thread = webrtc::Thread::Current();
  pcf_deps.network_thread = webrtc::Thread::Current();
  pcf_deps.worker_thread = webrtc::Thread::Current();
  pcf_deps.event_log_factory = std::make_unique<webrtc::RtcEventLogFactory>();
  CreateSomeMediaDeps(pcf_deps);
  webrtc::EnableMedia(pcf_deps);
  return pcf_deps;
}

bool de::stream_webrtc::CPeerConnectionManager::CreatePeerConnection(const std::string &sessionID, const std::string &peerID, const std::string &channelNumber, const std::string &channelName) 
{
	
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    
    de::CConfigFile &cConfigFile = CConfigFile::getInstance();
    const Json_de& jsonConfig = cConfigFile.GetConfigJSON();
    
    Json_de jsonIceServers= jsonConfig.contains("iceServers")?jsonConfig["iceServers"]:Json_de();
    webrtc::PeerConnectionInterface::IceServer turn_server;
    webrtc::PeerConnectionInterface::IceServer stun_server;
    
    // YOU SHOULD UNCOMMENT THIS AND USE ADDTRACK DIRECTLY
    // https://developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/addStream
  
	// Add the turn server. 
    m_config.servers.clear();
	for (auto iceServer : jsonIceServers)
	{
		ICE_SERVER server; // = getIceServerFromUrl(iceServer["uri"].get<std::string>());
		//BUG: could be a bug here ... check examples/unityplugin/simple_peer_connection.cc line 191
		//server.uri = iceServer["uri"].get<std::string>();
        server.tls_cert_policy = webrtc::PeerConnectionInterface::TlsCertPolicy::kTlsCertPolicyInsecureNoCheck;
        server.urls.push_back(iceServer["urls"].get<std::string>()); // = iceServer["urls"].get<std::string>();
		server.hostname = iceServer["urls"].get<std::string>();
        if (server.hostname.empty()) continue; // handle leaving extra comma in the config file after the last entry
        if (iceServer.contains("username"))
		{
			server.username = iceServer["username"].get<std::string>();
			server.password = iceServer.contains("password")?iceServer["password"].get<std::string>():"";
		}
        m_config.servers.push_back(server);
	}
    
    m_config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;  // using planB makes offer Bundle writes video.
    
    
    
    auto pcf_deps = CreateSomePcfDeps();
    auto peer_connection_deps = webrtc::PeerConnectionDependencies(this);
    auto peer_connection_or_error =
        de::stream_webrtc::CUserMedia::GetPeerConnectionFactory().get()->CreatePeerConnectionOrError(
                m_config, std::move(peer_connection_deps)); 

    // Handle the RTCErrorOr result
    if (peer_connection_or_error.ok()) {
        m_peerConnection = peer_connection_or_error.MoveValue();
    } else {
        // Handle error: peer_connection_or_error.error().message()
        RTC_LOG(LS_ERROR) << "Failed to create peer connection: " << peer_connection_or_error.error().message();
        m_peerConnection = nullptr; // Explicitly set to nullptr on failure
        // Consider returning an error code or throwing an exception here
    }
    
    m_sessionID = sessionID;
    m_peerID = peerID;
    m_channelNumber = channelName;
    m_channelName = channelName;

    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

	return m_peerConnection.get() != nullptr;;
}	

webrtc::RTCErrorOr<webrtc::scoped_refptr<webrtc::RtpTransceiverInterface>> de::stream_webrtc::CPeerConnectionManager::AddTransceiver(
    webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track)
{
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

	webrtc::RTCErrorOr<webrtc::scoped_refptr<webrtc::RtpTransceiverInterface>> result =
      m_peerConnection.get()->AddTransceiver(track);
    
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

	return result;
}
bool de::stream_webrtc::CPeerConnectionManager::AddStream(webrtc::MediaStreamInterface *stream) {
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    if (!m_peerConnection.get())
        return false;

    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

	
    return m_peerConnection.get()->AddStream(stream);

}


void de::stream_webrtc::CPeerConnectionManager::AddTrack(webrtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
    const std::vector<std::string>& stream_ids) {
   
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    if (!m_peerConnection.get())
        return ;
   
    m_peerConnection.get()->AddTrack(track, stream_ids);

    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}

bool de::stream_webrtc::CPeerConnectionManager::CreateOffer() {
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    if (!m_peerConnection.get())
        return false;

    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;

    options.offer_to_receive_audio = 0;
    options.offer_to_receive_video = 0;
    options.ice_restart=true;
    options.use_rtp_mux = true;
    options.voice_activity_detection = false;

    m_peerConnection.get()->CreateOffer((CreateSessionDescriptionObserver*)this, options);

    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << _LOG_CONSOLE_BOLD_TEXT << " DEBUG: CreateOffer" << _NORMAL_CONSOLE_TEXT_ << std::endl;
      
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    
    return true;
}

bool de::stream_webrtc::CPeerConnectionManager::CreateAnswer() {
    // if (!m_peerConnection.get())
    // return false;

    // webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;
    // bool mandatory_receive_ = false;
    // if (mandatory_receive_) {
    //     options.offer_to_receive_audio = true;
    //     options.offer_to_receive_video = true;
    // }
    
    
    // m_peerConnection.get()->CreateAnswer((CreateSessionDescriptionObserver*)this, options);

    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    
    return true;
}


void de::stream_webrtc::CPeerConnectionManager::SetRemoteDescription(const std::string& type, const std::string& sdp)
{

    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    std::unique_ptr<webrtc::SessionDescriptionInterface> session_description = webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp);
            
    auto  observer = CSetSessionDescriptionObserver::Create(this);
    observer->no = true;
    m_peerConnection.get()->SetRemoteDescription(observer,session_description.release());

    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}

  
bool de::stream_webrtc::CPeerConnectionManager::AddIceCandidate (const Json_de& packet)
{
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

	webrtc::SdpParseError error;
    
    std::string SdpName = "";
    std::string SdpMidName = "";
    std::string SdpMlineIndexName = "";
    
    if (packet.contains(de::stream_webrtc::kCandidateSdpName))
    {
        SdpName = packet[de::stream_webrtc::kCandidateSdpName].get<std::string>();
    }
    
    if (packet.contains(de::stream_webrtc::kCandidateSdpMidName))
    {
        SdpMidName = packet[de::stream_webrtc::kCandidateSdpMidName].get<std::string>();
    }
    
    if (packet.contains(de::stream_webrtc::kCandidateSdpMlineIndexName))
    {
        SdpMlineIndexName = packet[de::stream_webrtc::kCandidateSdpMlineIndexName].get<int>();
    }

    std::unique_ptr<webrtc::IceCandidateInterface> candidate(
            webrtc::CreateIceCandidate(SdpMidName, std::atoi(SdpMlineIndexName.c_str()), SdpName,  &error));
    
    if (!candidate.get()) {
        RTC_LOG(LS_WARNING) << "Can't parse received candidate message. "
                       << "SdpParseError was: " << error.description;
        return false;
    }

    if (!m_peerConnection.get()->AddIceCandidate (candidate.get()))
    {
        RTC_LOG(LS_WARNING) << "Failed in AddIceCandidate";
    
        return false;
    }

    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

	
    return true;
}

void de::stream_webrtc::CPeerConnectionManager::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
    
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

	
    std::string sdp;
    desc->ToString(&sdp);

    RTC_LOG(LS_INFO) << "PeerConnectionTestWrapper " <<  ": "
                     << webrtc::SdpTypeToString(desc->GetType())
                     << " sdp created: " << sdp
                     << " type created: " << desc->type();

  
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  " << _LOG_CONSOLE_BOLD_TEXT << " DEBUG: CPeerConnectionManager::OnSuccess" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    webrtc::SdpParseError error;
    
    m_peerConnection.get()->SetLocalDescription(
                CSetSessionDescriptionObserver::Create(this), desc);

    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

	return ;
}

// FROM:         
//      SetSessionDescriptionObserver AND
//      CreateSessionDescriptionObserver
void de::stream_webrtc::CPeerConnectionManager::OnFailure(webrtc::RTCError error) {
    
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

	std::cout << ToString(error.type()) << ": " << error.message() << std::endl;

  // TODO(hta): include error.type in the message
 
    std::cout << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << "\033[1;31m" << " DEBUG: CPeerConnectionManager::OnFailure" << ToString(error.type()) << ": " << error.message() << _NORMAL_CONSOLE_TEXT_ << std::endl;

	#ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

	return ;
}


/**
 * @brief Offer has been successfully created.
 * 
 */
void de::stream_webrtc::CPeerConnectionManager::onsuccess() {

    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

	
    if (m_callbacks)
    {
        std::string type;

        std::string sdp;
        m_peerConnection->local_description()->ToString(&sdp);
        type = webrtc::SdpTypeToString(m_peerConnection->local_description()->GetType()); 

        m_callbacks->OnLocalSdpReadytoSend(m_sessionID.c_str(), type.c_str(), sdp.c_str());
    }

    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

	
}


void de::stream_webrtc::CPeerConnectionManager::onfailed() {
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << __LINE__ << _LOG_CONSOLE_BOLD_TEXT << " DEBUG: SetLocalDescripton::onfailed" << _NORMAL_CONSOLE_TEXT_ << std::endl;
}


void de::stream_webrtc::CPeerConnectionManager::Close() {
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

	
    std::cout << __FILE__ << "." << __FUNCTION__ << __LINE__ << _LOG_CONSOLE_BOLD_TEXT << " DEBUG: CPeerConnectionManager::Close" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    m_peerConnection->Close();
    m_peerConnection = nullptr;

    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

}

// Triggered when the SignalingState changed.
void de::stream_webrtc::CPeerConnectionManager::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state)
{
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

	switch (new_state)
    {
        case webrtc::PeerConnectionInterface::kStable:
            std::cout << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: OnSignalingChange:kStable" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            break;
        
        case webrtc::PeerConnectionInterface::kClosed:
            std::cout << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: OnSignalingChange:CLOSED" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            break;
        
        case webrtc::PeerConnectionInterface::kHaveLocalOffer:
            std::cout << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: OnSignalingChange:kHaveLocalOffer" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            break;
        
        case webrtc::PeerConnectionInterface::kHaveLocalPrAnswer:
            std::cout << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: OnSignalingChange:kHaveLocalPrAnswer" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            break;
        
        case webrtc::PeerConnectionInterface::kHaveRemoteOffer:
            std::cout << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: OnSignalingChange:kHaveRemoteOffer" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            break;
        
        case webrtc::PeerConnectionInterface::kHaveRemotePrAnswer:
            std::cout << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: OnSignalingChange:kHaveRemotePrAnswer" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            break;
        
        default:
            std::cout << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: OnSignalingChange:???????" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            break;
    }

    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

   
}

// Triggered when media is received on a new stream from remote peer.
void de::stream_webrtc::CPeerConnectionManager::OnAddStream(webrtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
        
}

// Triggered when a remote peer closes a stream.
void de::stream_webrtc::CPeerConnectionManager::OnRemoveStream(webrtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
        
}

// Triggered when a remote peer opens a data channel.
void de::stream_webrtc::CPeerConnectionManager::OnDataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> data_channel)
{
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}

// Triggered when renegotiation is needed. For example, an ICE restart
// has begun.
void de::stream_webrtc::CPeerConnectionManager::OnRenegotiationNeeded()
{
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}


void de::stream_webrtc::CPeerConnectionManager::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state)
{
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    switch (new_state)
    {
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionNew:
            std::cout << "kIceConnectionNew";
            break;
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionChecking:
            std::cout << "kIceConnectionChecking";
            break;
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionConnected:
            std::cout << "kIceConnectionConnected";
            break;
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionCompleted:
            std::cout << "kIceConnectionCompleted";
            break;
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionFailed:
            std::cout << "kIceConnectionFailed";
            break;
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionDisconnected:
            std::cout << "kIceConnectionDisconnected";
            std::cout << _NORMAL_CONSOLE_TEXT_ << std::endl;
            //TODO: Handle  disconnect state ex. if browser refresh.
            //This is called when other side terminates the connection aggressivly e.g. refresh browser.
            if (m_callbacks)
            {
                m_callbacks->OnIceConnectionDisconnected(m_sessionID);
            }
//            this->Close();
            break;
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionClosed:
            std::cout << "kIceConnectionClosed";
            break;
        case webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionMax:
            std::cout << "kIceConnectionMax";
            break;
    }


    std::cout << _NORMAL_CONSOLE_TEXT_ << std::endl;
    
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}

// Called any time the PeerConnectionState changes.
void de::stream_webrtc::CPeerConnectionManager::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state)
{   
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    
    switch (new_state)
    {
    case webrtc::PeerConnectionInterface::PeerConnectionState::kNew:
        std::cout << "kNew";
    break;
    case webrtc::PeerConnectionInterface::PeerConnectionState::kConnecting:
        std::cout << "kConnecting";
    break;
    case webrtc::PeerConnectionInterface::PeerConnectionState::kConnected:
        std::cout << "kConnected";
    break;
    case webrtc::PeerConnectionInterface::PeerConnectionState::kDisconnected:
        std::cout << "kDisconnected";
    break;
    case webrtc::PeerConnectionInterface::PeerConnectionState::kFailed:
        std::cout << "kFailed";
    break;
    case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
        std::cout << "kClosed";
    break;
    
    default:
        std::cout << "UNKNOWN";
        break;
    }
    
    std::cout << _NORMAL_CONSOLE_TEXT_ << std::endl;

    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}


// Called any time the IceGatheringState changes.
void de::stream_webrtc::CPeerConnectionManager::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
{ 
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    switch (new_state)
    { 
        case webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringNew:
            std::cout << "kIceGatheringNew";
            break;
        
        case webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringGathering:
            std::cout << "kIceGatheringGathering";
            break;
        
        case webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringComplete:
            std::cout << "kIceGatheringComplete" << _NORMAL_CONSOLE_TEXT_;
            break;
        
        default:
            std::cout << "UNKNOWN";
            break;
    
    }

    std::cout  << _NORMAL_CONSOLE_TEXT_ << std::endl;
    
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}


// A new ICE candidate has been gathered.
void de::stream_webrtc::CPeerConnectionManager::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{ 
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    if (m_callbacks)
    {
        m_callbacks->OnIceCandidate(m_sessionID, candidate);
    }

    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
}


// Ice candidates have been removed.
// TODO(honghaiz): Make this a pure virtual method when all its subclasses
// implement it.
void de::stream_webrtc::CPeerConnectionManager::OnIceCandidatesRemoved(const std::vector<webrtc::Candidate>& candidates)
{ 
   #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
   #endif
}


// Called when the ICE connection receiving status changes.
void de::stream_webrtc::CPeerConnectionManager::OnIceConnectionReceivingChange(bool receiving)
{ 
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}



// This is called when a receiver and its track are created.
// TODO(zhihuang): Make this pure virtual when all subclasses implement it.
// Note: This is called with both Plan B and Unified Plan semantics. Unified
// Plan users should prefer OnTrack, OnAddTrack is only called as backwards
// compatibility (and is called in the exact same situations as OnTrack).
void de::stream_webrtc::CPeerConnectionManager::OnAddTrack(webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                const std::vector<webrtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams)
{ 
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}


// This is called when signaling indicates a transceiver will be receiving
// media from the remote endpoint. This is fired during a call to
// SetRemoteDescription. The receiving track can be accessed by:
// |transceiver->receiver()->track()| and its associated streams by
// |transceiver->receiver()->streams()|.
// Note: This will only be called if Unified Plan semantics are specified.
// This behavior is specified in section 2.2.8.2.5 of the "Set the
// RTCSessionDescription" algorithm:
// https://w3c.github.io/webrtc-pc/#set-description
void de::stream_webrtc::CPeerConnectionManager::OnTrack(webrtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{ 
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}


// Called when signaling indicates that media will no longer be received on a
// track.
// With Plan B semantics, the given receiver will have been removed from the
// PeerConnection and the track muted.
// With Unified Plan semantics, the receiver will remain but the transceiver
// will have changed direction to either sendonly or inactive.
// https://w3c.github.io/webrtc-pc/#process-remote-track-removal
// TODO(hbos,deadbeef): Make pure virtual when all subclasses implement it.
void de::stream_webrtc::CPeerConnectionManager::OnRemoveTrack(webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{ 
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}


// Called when an interesting usage is detected by WebRTC.
// An appropriate action is to add information about the context of the
// PeerConnection and write the event to some kind of "interesting events"
// log function.
// The heuristics for defining what constitutes "interesting" are
// implementation-defined.
void de::stream_webrtc::CPeerConnectionManager::OnInterestingUsage(int usage_pattern)
{ 
    #ifdef DDEBUG
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}



