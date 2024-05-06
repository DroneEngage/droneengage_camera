
#include "../common.h"


#include "../helpers/json_nlohmann.hpp"
using Json_de = nlohmann::json;


static const rtc::SocketAddress kDefaultLocalAddress("192.168.1.139", 1);


uavos::stream_webrtc::CPeerConnectionManager::CPeerConnectionManager ():m_callbacks(NULL)
{
}

uavos::stream_webrtc::CPeerConnectionManager::CPeerConnectionManager (CCallbacks *callbacks):m_callbacks(callbacks)
{
}


uavos::stream_webrtc::CPeerConnectionManager::~CPeerConnectionManager()
{
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << "\033[1;31m" << "DESTRUCTOR" << _NORMAL_CONSOLE_TEXT_ << std::endl;
}


bool uavos::stream_webrtc::CPeerConnectionManager::CreatePeerConnection(const std::string &sessionID, const std::string &peerID, const std::string &channelNumber, const std::string &channelName) 
{
	
    
    uavos::CConfigFile &cConfigFile = CConfigFile::getInstance();
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
    //m_config.enable_ice_renomination = true;
    //// WHEN ENABLED NO ONICECANDIDATE is triggered.
    // 1- m_config.tcp_candidate_policy = webrtc::PeerConnectionInterface::TcpCandidatePolicy::kTcpCandidatePolicyDisabled; 
    /////
    m_config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;  // using planB makes offer Bundle writes video.
    
    // stranges:
    // m_config.bundle_policy = webrtc::PeerConnectionInterface::BundlePolicy::kBundlePolicyMaxCompat;
    //m_config.disable_ipv6 = true;
    //m_config.disable_ipv6_on_wifi = false;
    
    // BAD CHOICE m_config.use_media_transport_for_data_channels = true;
    // BAD CHOICE m_config.use_datagram_transport = true;
    //m_config.prune_turn_ports = true;
    
    
    uavos::CNetworkManager * fnm = new uavos::CNetworkManager();
    //fnm->AddInterface (kDefaultLocalAddress,"wifi",rtc::AdapterType::ADAPTER_TYPE_WIFI);   // << IP ADDED TO HOST BUT TCP NOT UDP !!!
    
    //rtc::BasicNetworkManager * bnm = new rtc::BasicNetworkManager();
    cricket::BasicPortAllocator* bpa = new cricket::BasicPortAllocator(fnm);
    std::unique_ptr<cricket::PortAllocator> port_allocator(bpa);
    // port_allocator->set_flags(cricket::PORTALLOCATOR_DISABLE_TCP |
    //                           cricket::PORTALLOCATOR_DISABLE_RELAY);

    
    // const cricket::FakePortAllocatorSession* session =
    //   static_cast<const cricket::FakePortAllocatorSession*>(
    //       port_allocator.get()->GetPooledSession());
    

    RTC_LOG(INFO) << __FUNCTION__ << "CreatePeerConnection peerid:" << peerID;
    m_peerConnection = uavos::stream_webrtc::CUserMedia::GetPeerConnectionFactory().get()->CreatePeerConnection(
                m_config, std::move(port_allocator) // */
                , nullptr, (PeerConnectionObserver*)this);

    m_sessionID = sessionID;
    m_peerID = peerID;
    m_channelNumber = channelName;
    m_channelName = channelName;

	return m_peerConnection.get() != nullptr;;
}	

webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpTransceiverInterface>> uavos::stream_webrtc::CPeerConnectionManager::AddTransceiver(
    rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track)
{
    webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpTransceiverInterface>> result =
      m_peerConnection.get()->AddTransceiver(track);
      return result;
}
bool uavos::stream_webrtc::CPeerConnectionManager::AddStream(webrtc::MediaStreamInterface *stream) {
    if (!m_peerConnection.get())
        return false;
    return m_peerConnection.get()->AddStream(stream);

}


void uavos::stream_webrtc::CPeerConnectionManager::AddTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
    const std::vector<std::string>& stream_ids) {
   
    if (!m_peerConnection.get())
        return ;
   
    m_peerConnection.get()->AddTrack(track, stream_ids);

}

bool uavos::stream_webrtc::CPeerConnectionManager::CreateOffer() {
    if (!m_peerConnection.get())
        return false;

    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;

    options.offer_to_receive_audio = 0;
    options.offer_to_receive_video = 0;
    options.ice_restart=true;
    options.use_rtp_mux = true;
    options.voice_activity_detection = false;

    m_peerConnection.get()->CreateOffer((CreateSessionDescriptionObserver*)this, options);

    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << _LOG_CONSOLE_TEXT_BOLD_ << " DEBUG: CreateOffer" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
    return true;
}

bool uavos::stream_webrtc::CPeerConnectionManager::CreateAnswer() {
    // if (!m_peerConnection.get())
    // return false;

    // webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;
    // bool mandatory_receive_ = false;
    // if (mandatory_receive_) {
    //     options.offer_to_receive_audio = true;
    //     options.offer_to_receive_video = true;
    // }
    
    
    // m_peerConnection.get()->CreateAnswer((CreateSessionDescriptionObserver*)this, options);
    return true;
}


void uavos::stream_webrtc::CPeerConnectionManager::SetRemoteDescription(const std::string& type, const std::string& sdp)
{

    #ifdef DEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << _LOG_CONSOLE_TEXT_BOLD_ << " DEBUG: SetRemoteDescription" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    std::unique_ptr<webrtc::SessionDescriptionInterface> session_description = 
            webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp);
    rtc::scoped_refptr<CSetSessionDescriptionObserver> observer = new rtc::RefCountedObject<CSetSessionDescriptionObserver>(this);
    observer.get()->no = true;
    m_peerConnection.get()->SetRemoteDescription(observer.get(),session_description.release());
}

  
bool uavos::stream_webrtc::CPeerConnectionManager::AddIceCandidate (const Json_de& packet)
{
    webrtc::SdpParseError error;
    
    std::string SdpName = "";
    std::string SdpMidName = "";
    std::string SdpMlineIndexName = "";
    
    if (packet.contains(uavos::stream_webrtc::kCandidateSdpName))
    {
        SdpName = packet[uavos::stream_webrtc::kCandidateSdpName].get<std::string>();
    }
    
    if (packet.contains(uavos::stream_webrtc::kCandidateSdpMidName))
    {
        SdpMidName = packet[uavos::stream_webrtc::kCandidateSdpMidName].get<std::string>();
    }
    
    if (packet.contains(uavos::stream_webrtc::kCandidateSdpMlineIndexName))
    {
        SdpMlineIndexName = packet[uavos::stream_webrtc::kCandidateSdpMlineIndexName].get<int>();
    }

    std::unique_ptr<webrtc::IceCandidateInterface> candidate(
            webrtc::CreateIceCandidate(SdpMidName, std::atoi(SdpMlineIndexName.c_str()), SdpName,  &error));
    
    if (!candidate.get()) {
      RTC_LOG(WARNING) << "Can't parse received candidate message. "
                       << "SdpParseError was: " << error.description;
      return false;
    }

    if (!m_peerConnection.get()->AddIceCandidate (candidate.get()))
    {
        RTC_LOG(WARNING) << "Failed in AddIceCandidate";
      
    }
    return true;
}

void uavos::stream_webrtc::CPeerConnectionManager::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
    
    std::string sdp;
    desc->ToString(&sdp);
    // m_test_sdb = sdp.c_str();
    // m_test_type = webrtc::SdpTypeToString(desc->GetType()); //.c_str();

    RTC_LOG(LS_INFO) << "PeerConnectionTestWrapper " <<  ": "
                     << webrtc::SdpTypeToString(desc->GetType())
                     << " sdp created: " << sdp
                     << " type created: " << desc->type();

  
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  " << _LOG_CONSOLE_TEXT_BOLD_ << " DEBUG: CPeerConnectionManager::OnSuccess" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    webrtc::SdpParseError error;
    // webrtc::SessionDescriptionInterface *session_description = 
    //             webrtc::CreateSessionDescription(desc->type(), sdp, &error);
    

    //std::unique_ptr<webrtc::IceCandidateInterface> candidate(cricket::Candidate());
    cricket::Candidate candidate;
    candidate.set_component(cricket::ICE_CANDIDATE_COMPONENT_DEFAULT);
    candidate.set_protocol(cricket::UDP_PROTOCOL_NAME);
    candidate.set_address(kDefaultLocalAddress);
    candidate.set_type(cricket::LOCAL_PORT_TYPE);
    


    
    m_peerConnection.get()->SetLocalDescription(
                CSetSessionDescriptionObserver::Create(this), desc);

    // if (m_callbacks)
    // {
    //     m_callbacks->OnLocalSdpReadytoSend(m_sessionID.c_str(), desc->type().c_str(), sdp.c_str());
    // }

    return ;
}

// FROM:         
//      SetSessionDescriptionObserver AND
//      CreateSessionDescriptionObserver
void uavos::stream_webrtc::CPeerConnectionManager::OnFailure(webrtc::RTCError error) {
    std::cout << ToString(error.type()) << ": " << error.message() << std::endl;

  // TODO(hta): include error.type in the message
 
    std::cout << __FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << "\033[1;31m" << " DEBUG: CPeerConnectionManager::OnFailure" << ToString(error.type()) << ": " << error.message() << _NORMAL_CONSOLE_TEXT_ << std::endl;

	return ;
}


/**
 * @brief Offer has been successfully created.
 * 
 */
void uavos::stream_webrtc::CPeerConnectionManager::onsuccess() {

    std::cout << __FILE__ << "." << __FUNCTION__ << __LINE__ << _LOG_CONSOLE_TEXT_BOLD_ << " DEBUG: SetLocalDescripton::onsuccess" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    
    if (m_callbacks)
    {
        std::string type;

        std::string sdp;
        m_peerConnection->local_description()->ToString(&sdp);
        type = webrtc::SdpTypeToString(m_peerConnection->local_description()->GetType()); 

        m_callbacks->OnLocalSdpReadytoSend(m_sessionID.c_str(), type.c_str(), sdp.c_str());
    }
}


void uavos::stream_webrtc::CPeerConnectionManager::onfailed() {
    std::cout << __FILE__ << "." << __FUNCTION__ << __LINE__ << _LOG_CONSOLE_TEXT_BOLD_ << " DEBUG: SetLocalDescripton::onfailed" << _NORMAL_CONSOLE_TEXT_ << std::endl;
}


void uavos::stream_webrtc::CPeerConnectionManager::Close() {
    std::cout << __FILE__ << "." << __FUNCTION__ << __LINE__ << _LOG_CONSOLE_TEXT_BOLD_ << " DEBUG: CPeerConnectionManager::Close" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    m_peerConnection->Close();
    m_peerConnection = nullptr;
    
}

// Triggered when the SignalingState changed.
void uavos::stream_webrtc::CPeerConnectionManager::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state)
{
    switch (new_state)
    {
        case webrtc::PeerConnectionInterface::kStable:
            std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnSignalingChange:kStable" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            break;
        
        case webrtc::PeerConnectionInterface::kClosed:
            std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnSignalingChange:CLOSED" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            break;
        
        case webrtc::PeerConnectionInterface::kHaveLocalOffer:
            std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnSignalingChange:kHaveLocalOffer" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            break;
        
        case webrtc::PeerConnectionInterface::kHaveLocalPrAnswer:
            std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnSignalingChange:kHaveLocalPrAnswer" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            break;
        
        case webrtc::PeerConnectionInterface::kHaveRemoteOffer:
            std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnSignalingChange:kHaveRemoteOffer" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            break;
        
        case webrtc::PeerConnectionInterface::kHaveRemotePrAnswer:
            std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnSignalingChange:kHaveRemotePrAnswer" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            break;
        
        default:
            std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnSignalingChange:???????" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            break;
    }

   
}

// Triggered when media is received on a new stream from remote peer.
void uavos::stream_webrtc::CPeerConnectionManager::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
    #ifdef DEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnAddStream" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
        
}

// Triggered when a remote peer closes a stream.
void uavos::stream_webrtc::CPeerConnectionManager::OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream)
{
    #ifdef DEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnRemoveStream" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
        
}

// Triggered when a remote peer opens a data channel.
void uavos::stream_webrtc::CPeerConnectionManager::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel)
{
    #ifdef DEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnDataChannel" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}

// Triggered when renegotiation is needed. For example, an ICE restart
// has begun.
void uavos::stream_webrtc::CPeerConnectionManager::OnRenegotiationNeeded()
{
    #ifdef DEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnRenegotiationNeeded" << _NORMAL_CONSOLE_TEXT_ << std::endl;
	#endif
}


void uavos::stream_webrtc::CPeerConnectionManager::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state)
{
    #ifdef DEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnIceConnectionChange state: "  << _INFO_CONSOLE_TEXT << std::endl;
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
    
    
}

// Called any time the PeerConnectionState changes.
void uavos::stream_webrtc::CPeerConnectionManager::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state)
{   
    #ifdef DEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnConnectionChange " << "\033[1;33m";
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
}


// Called any time the IceGatheringState changes.
void uavos::stream_webrtc::CPeerConnectionManager::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
{ 
    #ifdef DEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnIceGatheringChange " << "\033[1;33m";
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
    
}


// A new ICE candidate has been gathered.
void uavos::stream_webrtc::CPeerConnectionManager::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{ 
    #ifdef DEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << _LOG_CONSOLE_TEXT_BOLD_ << " DEBUG: OnIceCandidate" << _NORMAL_CONSOLE_TEXT_ << std::endl ;
    #endif

    if (m_callbacks)
    {
        m_callbacks->OnIceCandidate(m_sessionID, candidate);
    }
    
}


// Ice candidates have been removed.
// TODO(honghaiz): Make this a pure virtual method when all its subclasses
// implement it.
void uavos::stream_webrtc::CPeerConnectionManager::OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& candidates)
{ 
   #ifdef DEBUG
   std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  " << _LOG_CONSOLE_TEXT_BOLD_ << " DEBUG: OnIceCandidatesRemoved" << _NORMAL_CONSOLE_TEXT_ << std::endl;
   #endif
}


// Called when the ICE connection receiving status changes.
void uavos::stream_webrtc::CPeerConnectionManager::OnIceConnectionReceivingChange(bool receiving)
{ 
    #ifdef DEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  " << _LOG_CONSOLE_TEXT_BOLD_ << " DEBUG: OnIceConnectionReceivingChange" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}



// This is called when a receiver and its track are created.
// TODO(zhihuang): Make this pure virtual when all subclasses implement it.
// Note: This is called with both Plan B and Unified Plan semantics. Unified
// Plan users should prefer OnTrack, OnAddTrack is only called as backwards
// compatibility (and is called in the exact same situations as OnTrack).
void uavos::stream_webrtc::CPeerConnectionManager::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams)
{ 
    #ifdef DEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  " << _LOG_CONSOLE_TEXT_BOLD_ << " DEBUG: OnAddTrack" << _NORMAL_CONSOLE_TEXT_ << std::endl;
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
void uavos::stream_webrtc::CPeerConnectionManager::OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver)
{ 
    #ifdef DEBUG
    std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: OnTrack" << _NORMAL_CONSOLE_TEXT_ << std::endl;
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
void uavos::stream_webrtc::CPeerConnectionManager::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{ 
    #ifdef DEBUG
    std::cout << _LOG_CONSOLE_TEXT_BOLD_ << " DEBUG: OnRemoveTrack" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}


// Called when an interesting usage is detected by WebRTC.
// An appropriate action is to add information about the context of the
// PeerConnection and write the event to some kind of "interesting events"
// log function.
// The heuristics for defining what constitutes "interesting" are
// implementation-defined.
void uavos::stream_webrtc::CPeerConnectionManager::OnInterestingUsage(int usage_pattern)
{ 
    #ifdef DEBUG
    std::cout << _LOG_CONSOLE_TEXT_BOLD_ << " DEBUG: OnInterestingUsage" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}



