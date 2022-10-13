
#include "common.h"
#include <fstream>



extern std::string PartyID;

uavos::CWEBRTC_Plugin::~CWEBRTC_Plugin ()
{
    m_videoDeviceInfoList.clear();
    m_audioDeviceInfoList.clear();
}


void uavos::CWEBRTC_Plugin::initCameras(const bool singleCameraMode)
{
    filled = ATOMIC_VAR_INIT(false);
        
    m_singleCameraMode = singleCameraMode;

    // get available video devices count
    m_actualVideoSourcesCount = uavos::stream_webrtc::CSource::GetVideoSourcesCount();

    if (m_actualVideoSourcesCount < 0)
    {
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "Could not access any video devices." << _NORMAL_CONSOLE_TEXT_ << std::endl;

        exit(0);
    }

    std::cout << _SUCCESS_CONSOLE_TEXT_ << "========================================================"  << _NORMAL_CONSOLE_TEXT_ << std::endl;
    std::cout << "No. of Video devices "  << _SUCCESS_CONSOLE_BOLD_TEXT_ << m_actualVideoSourcesCount << _NORMAL_CONSOLE_TEXT_ << std::endl;
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Please check below list to match config file for selected cameras"  << _NORMAL_CONSOLE_TEXT_ << std::endl;

    
    
    m_videoDeviceInfoList.clear();
    
    // get all devices available.
    uavos::stream_webrtc::CSource::GetDevices(m_videoDeviceInfoList,m_actualVideoSourcesCount);

    // Write Available Camera(s) in Console.
    for (uint i=0; i<m_actualVideoSourcesCount; ++i)
    {
        m_videoDeviceInfoList[i].selected = false;
        m_videoDeviceInfoList[i].active = 0;
        std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Valid Video Device Found: "  
        << _SUCCESS_CONSOLE_TEXT_ << " /dev/video:" << _INFO_BOLD_CONSOLE_TEXT << m_videoDeviceInfoList[i].dev_linux_number 
        << _SUCCESS_CONSOLE_TEXT_ << " device index:" << _INFO_BOLD_CONSOLE_TEXT << m_videoDeviceInfoList[i].device_num 
        << _SUCCESS_CONSOLE_TEXT_ << " device name:" << _INFO_BOLD_CONSOLE_TEXT << m_videoDeviceInfoList[i].device_name 
        << _SUCCESS_CONSOLE_TEXT_ << " device id:" << _INFO_BOLD_CONSOLE_TEXT << m_videoDeviceInfoList[i].device_id 
        << _NORMAL_CONSOLE_TEXT_ << std::endl;
    }

    std::cout << _SUCCESS_CONSOLE_TEXT_ << "========================================================"  << _NORMAL_CONSOLE_TEXT_ << std::endl;
}

void uavos::CWEBRTC_Plugin::InitializePeerConnection()
{
    m_connection = rtc::scoped_refptr<uavos::stream_webrtc::CUserMedia>(new rtc::RefCountedObject<uavos::stream_webrtc::CUserMedia>());
    m_connection->InitializePeerConnection();
}

/**
 * @brief 
 * 
 * @param cameraVideoName 
 * @param cameraVideoIndex equals to /dev/video## where ## equals to cameraVideoIndex
 * @return true 
 * @return false 
 */
bool uavos::CWEBRTC_Plugin::addCameraByID (std::string cameraVideoName, int cameraVideoIndex)
{
    
    
    //if (cameraVideoIndex > m_actualVideoSourcesCount) return false; // out of range
    for (int i=0; i<=m_actualVideoSourcesCount; ++i)
    {
        if (m_videoDeviceInfoList[i].dev_linux_number == cameraVideoIndex)
        {
            m_videoDeviceInfoList[i].selected = true;
            m_videoDeviceInfoList[i].active = 0;
            m_videoDeviceInfoList[i].local_name = cameraVideoName;
    
            std::cout << _SUCCESS_CONSOLE_TEXT_ << "Camera: " << _SUCCESS_CONSOLE_BOLD_TEXT_ << cameraVideoName << _SUCCESS_CONSOLE_TEXT_ << " \\dev\\video" << _INFO_CONSOLE_TEXT << std::to_string(cameraVideoIndex) << _NORMAL_CONSOLE_TEXT_ << std::endl;
            filled = ATOMIC_VAR_INIT(true);

            return true;
        }
    }
    
    
    return false;
}

void uavos::CWEBRTC_Plugin::addCameraByRange(int startVideoIndex, int endVideoIndex)
{
    
    if (startVideoIndex < 0)
    { 
        startVideoIndex = MIN_CAMERA_INDEX;
    }

       
    
    if ((startVideoIndex > m_actualVideoSourcesCount) || (startVideoIndex > endVideoIndex))
    {
        std::cout   << _ERROR_CONSOLE_BOLD_TEXT_ << "Bad start or end video index" << _NORMAL_CONSOLE_TEXT_ << std::endl;

        exit(0);
    }
    
    if (m_actualVideoSourcesCount > 0)
    {
        filled = false;
        
        for (int i =0; i < m_actualVideoSourcesCount; ++i)
        {   // mark devices that are selected.
            std::cout << m_videoDeviceInfoList[i].device_num << "  dn:" << m_videoDeviceInfoList[i].device_name << " ln:" << m_videoDeviceInfoList[i].local_name << " did: " << m_videoDeviceInfoList[i].device_id << " pid:" << m_videoDeviceInfoList[i].product_id;
            const bool selected = (startVideoIndex <= i) && (endVideoIndex >=i);
            m_videoDeviceInfoList[i].selected = selected;
            m_videoDeviceInfoList[i].active = 0;
            if (selected)
            {
                std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << " Selected" << _NORMAL_CONSOLE_TEXT_;
            }
            else
            {
                std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << " Skipped" << _NORMAL_CONSOLE_TEXT_;
            }
            std::cout  << std::endl;
        }

        filled = true;

    }
    else
    {
        //std::cout   << _ERROR_CONSOLE_TEXT_ << "No available video devices." << _NORMAL_CONSOLE_TEXT_ << std::endl;
        exit(0);
    }
}


/**
 * @brief Sends Offer to receiver.
 * 
 * @param sessionID 
 * @param type 
 * @param sdp 
 */
void uavos::CWEBRTC_Plugin::OnLocalSdpReadytoSend (const char* sessionID, const char* type, const char* sdp)
{
    #ifdef DEBUG
    std::cout << _LOG_CONSOLE_TEXT << "DEBUG: OnLocalSdpReadytoSend" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    std::cout << _LOG_CONSOLE_TEXT << "OFFER:" << std::endl <<  _NORMAL_CONSOLE_TEXT_ << std::string(sdp) << std::endl;
    
    if (!m_sendSignalJMSG) return ;
    
    Json::Value packet;
    uavos::STRUCT_SESSION_INFO sessionInfo = m_SessionMap[sessionID];
    packet["packet"]  = Json::Value();
    packet["packet"]["sdp"] = sdp;
    packet["packet"]["type"] = type;
    packet["number"]  = PartyID; // sessionInfo.channelNumber;
    packet["channel"] = sessionInfo.channelName;
    
    Json::Value w;
    w["w"] = packet;
    m_sendSignalJMSG (sessionInfo.senderPartyID.c_str(),w);
}


void uavos::CWEBRTC_Plugin::OnIceCandidate (const std::string& sessionID, const webrtc::IceCandidateInterface* const candidate)
{

    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << _LOG_CONSOLE_TEXT_BOLD_ << " DEBUG: OnIceCandidate:" <<_NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    if (!m_sendSignalJMSG) return ;
    
    std::string sdp;
    std::string sdp_mid;
    int sdp_mline_index  = 0;
    
    Json::Value packet;
    uavos::STRUCT_SESSION_INFO sessionInfo = m_SessionMap[sessionID];
    packet["packet"]  = Json::Value();
    
    
    if (candidate != nullptr)
    {
        candidate->ToString(&sdp);
        sdp_mid = candidate->sdp_mid();
        sdp_mline_index = candidate->sdp_mline_index();
        packet["packet"][uavos::stream_webrtc::kCandidateSdpName] = sdp;
    
    }
    
    
    packet["packet"][uavos::stream_webrtc::kCandidateSdpMidName] = sdp_mid;
    packet["packet"][uavos::stream_webrtc::kCandidateSdpMlineIndexName] = sdp_mline_index;
    packet["number"]  = PartyID; 
    packet["channel"] = sessionInfo.channelName;
    
    Json::Value w;
    w["w"] = packet;
    m_sendSignalJMSG (sessionInfo.senderPartyID.c_str(),w);
}


void uavos::CWEBRTC_Plugin::OnIceConnectionDisconnected (const std::string& sessionID)
{
    

    // commented code below is how to call a function from a thread in webrtc
    // uavos::stream_webrtc::CUserMedia::g_signaling_thread->Invoke<void>(RTC_FROM_HERE,
    // [&] {
    //     std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: INSIDE THREAD" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
    //     // sessionInfo->peerConnectionManager->Close();
    //      // eraseSessionInfoBySessionID(sessionID.c_str());
    // });

    deleteMe.push_back(sessionID);
    
    
}

bool uavos::CWEBRTC_Plugin::IsCorrectCameraIndex (const int cameraIndex)
{
    return true;
}


/**
 * Called periodically from main thread.
 * */
void uavos::CWEBRTC_Plugin::cleaning()
{

    if (deleteMe.size () > 0)
    {
        #ifdef DEBUG
        std::cout << __FUNCTION__ << __LINE__ << "Key " << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: deleteMe Found" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif

        std::string sessionID =  deleteMe[0];
        deleteMe.pop_back();
        const STRUCT_SESSION_INFO * sessionInfo = findSessionInfoBySessionID(sessionID.c_str());
        if ( sessionInfo == NULL)
        {
            #ifdef DEBUG
            std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: Session not Found" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            #endif     
            
            return ;
        }

        sessionInfo->peerConnectionManager->Close();
        eraseSessionInfoBySessionID(sessionID.c_str());
    }
}

void uavos::CWEBRTC_Plugin::SendOffer (const std::string& senderPartyID, const std::string& sessionID,  const std::string& channelNumber,  const std::string& channelName)
{

    if (sessionID.empty())
    {
        std::cout << "Key " << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: null sessionID" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        return ;
    }


    if (m_singleCameraMode)
    {
        std::cout << "Key " << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: m_singleCameraMode" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        const STRUCT_SESSION_INFO * sessionInfo = findSessionInfoBySessionID(sessionID.c_str());
        if ( sessionInfo != NULL)
        {
            std::cout << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << sessionID << _NORMAL_CONSOLE_TEXT_ << " found" << std::endl;
            //m_SessionMap[sessionID].connection.get()->RemoveVideoTracks();
            //m_SessionMap[sessionID].connection.get()->RemoveAudioTracks();
            //m_SessionMap[sessionID].connection.get()->DeletePeerConnection();
            
        }
        else
        {
            /* code */
        }
        

        uavos::STRUCT_SESSION_INFO sessionInfoNew;
        sessionInfoNew.senderPartyID = senderPartyID;
        sessionInfoNew.sessionID = sessionID;
        sessionInfoNew.channelNumber = channelNumber;
        sessionInfoNew.channelName = channelName;
        
        uavos::stream_webrtc::STRUCT_DEVICE_INFO device_info = findDeviceInfoByLocalName(channelName.c_str());
        if (device_info.device_num == -1)
        {
            std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: Camera " << channelName.c_str() << " Not Found" << _NORMAL_CONSOLE_TEXT_ << std::endl;

            return ;
        }

        if ((device_info.active <= 0))
        {
            if (!device_info.capturer->StartCapture())
            {
                // failed to start capturer.
                // send error message.
                std::cout << _ERROR_CONSOLE_TEXT_ << "Capturer " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: Failed to start Capturer" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                return ;
            }
            else
            {
                std::cout <<  _SUCCESS_CONSOLE_TEXT_ << " Capturer " << _SUCCESS_CONSOLE_BOLD_TEXT_ << "DEBUG: Capturer started successfully" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                device_info.active += 1;    
                updateDeviceInfoByLocalName(channelName.c_str(), device_info);
            }
        }

        sessionInfoNew.peerConnectionManager = new rtc::RefCountedObject<uavos::stream_webrtc::CPeerConnectionManager>(this);
        sessionInfoNew.peerConnectionManager->CreatePeerConnection(sessionID, senderPartyID, channelName, channelNumber);
        
        rtc::scoped_refptr<uavos::stream_webrtc::CapturerTrackSource>capturerTrackSource = new rtc::RefCountedObject<uavos::stream_webrtc::CapturerTrackSource> (
                        device_info.capturer);
        rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrackInterface = m_connection->CreateVideoTrackInterface (
                        device_info.unique_name,capturerTrackSource);
        
        
        // initiate sending offer at the end of series of events.
        ///m_connection->CreateLocalMediaStream(sessionID.c_str());
        ///webrtc::MediaStreamInterface * stream = m_connection->GetMediaStream().get();
        ///m_connection->AddVideoTrack(videoTrackInterface.get());
        ///sessionInfoNew.peerConnectionManager->AddStream(stream);
        
        //capturerTrackSource.get()->Start();
        
        // IMPORTANT
        // This function creates a stream for this SINGLE TRACK. SO name the stream with the same of the track.
        // FIRE FOX has a bug that it overwrites track ids and label so naming stream like track helps.
        sessionInfoNew.peerConnectionManager->AddTrack(videoTrackInterface,{device_info.unique_name});
        
        sessionInfoNew.peerConnectionManager->CreateOffer();
        
        // make it last after setting all structure items as they will be copied.
        m_SessionMap.insert(std::make_pair(sessionID.c_str(),sessionInfoNew));
       

        // if (session == undefined)
        // {
        //     session = {};
        //     session.number      = number;
        //     session.channel     = channel;
        //     _sessions[number + channel] = session;
        //     //session.tag  = commObject;
        // }
        // session.tag  = commObject;
    }
    else
    {
        /* code */
    }
    
}

void uavos::CWEBRTC_Plugin::ProcessAnswer (
            const std::string& senderPartyID, const std::string& sessionID, 
            const std::string& channelNumber,  const std::string& channelName,
            const Json::Value& packet)
{

    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "  "  << _LOG_CONSOLE_TEXT << "DEBUG: CWEBRTC_Plugin::ProcessAnswer" << _NORMAL_CONSOLE_TEXT_ <<  packet.toStyledString() << std::endl;
    
    if (sessionID.empty())
    {
        std::cout << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: null sessionID" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        return ;
    }

    const STRUCT_SESSION_INFO * sessionInfo = findSessionInfoBySessionID(sessionID.c_str());
    if ( sessionInfo == NULL)
    {
        std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: Session not Found" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        return ;
    }

    if (!packet.isMember("type") || !packet.isMember("sdp"))
    {
        std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: NO TYPE Or SDP" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        return ;
    }

    uavos::stream_webrtc::CPeerConnectionManager  * const cPeerConnectionManager = (uavos::stream_webrtc::CPeerConnectionManager  *) m_SessionMap[sessionID].peerConnectionManager.get();
    
    cPeerConnectionManager->SetRemoteDescription(packet["type"].asCString(), packet["sdp"].asCString());
    
}

void uavos::CWEBRTC_Plugin::ProcessCandidate (
                const std::string& senderPartyID, 
                const std::string& sessionID,  
                const std::string& channelNumber, 
                const std::string& channelName,
                const Json::Value& packet)
{
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << _LOG_CONSOLE_TEXT << "DEBUG: CWEBRTC_Plugin::ProcessCandidate" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    
    if (sessionID.empty())
    {
        std::cout << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: null sessionID" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        return ;
    }

    const STRUCT_SESSION_INFO * sessionInfo = findSessionInfoBySessionID(sessionID.c_str());
    if ( sessionInfo == NULL)
    {
        std::cout << __FUNCTION__ << __LINE__ << "Key " << _LOG_CONSOLE_TEXT << "DEBUG: Session not Found" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        return ;
    }
    

    uavos::stream_webrtc::CPeerConnectionManager  * const cPeerConnectionManager = (uavos::stream_webrtc::CPeerConnectionManager  *) m_SessionMap[sessionID].peerConnectionManager.get();
    
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _LOG_CONSOLE_TEXT << "DEBUG: ProcessCandidate" << _NORMAL_CONSOLE_TEXT_ << std::endl << packet.toStyledString() << std::endl;
        
    cPeerConnectionManager->AddIceCandidate(packet);
}


void uavos::CWEBRTC_Plugin::Hangup (const std::string& senderPartyID, const std::string& sessionID,  const std::string& number,  const std::string& channel)
{

    std::cout << __FUNCTION__ << __LINE__ << _LOG_CONSOLE_TEXT << "DEBUG:senderPartyID" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
    if (sessionID.empty())
    {
        std::cout << "Key " << _LOG_CONSOLE_TEXT << "DEBUG: null sessionID" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        return ;
    }

    const STRUCT_SESSION_INFO * sessionInfo = findSessionInfoBySessionID(sessionID.c_str());
    if ( sessionInfo == NULL)
    {
        std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: Session not Found" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        return ;
    }
    
   
    sessionInfo->peerConnectionManager->Close();
    eraseSessionInfoBySessionID(sessionID.c_str());
    
    
    Json::Value packet;
    packet["packet"]  = Json::Value();
    packet["packet"]["hangup"] = true;
    packet["number"]  = PartyID; 
    packet["channel"] = channel;
    Json::Value w;
    w["w"] = packet;
    m_sendSignalJMSG(senderPartyID.c_str(),w);
}



void uavos::CWEBRTC_Plugin::ExecuteSignalCommand(const Json::Value &jMsg)
{
    // extract command
    const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    const Json::Value w = cmd["w"];
    std::string number = w["number"].asString();
    std::string channel = w["channel"].asString();
    std::string sender  = jMsg[ANDRUAV_PROTOCOL_SENDER].asString();
    std::string sessionID = (number + channel);
    
    // https://stackoverflow.com/questions/10699689/how-can-i-get-a-value-from-a-map
    const uavos::STRUCT_SESSION_INFO * sessionInfo = findSessionInfoBySessionID(sessionID.c_str());
    if ( sessionInfo == NULL)
    {
        std::cout << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << sessionID << _NORMAL_CONSOLE_TEXT_ << " not found" << std::endl;
    }
    else
    {
        std::cout << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << sessionID << _NORMAL_CONSOLE_TEXT_ << " found" << std::endl;
    }
    

    //std::cout << "Key " << cmd.toStyledString() << std::endl;
    Json::Value const packet = w["packet"];

    if ((packet.isMember("joinme")==true) && (packet["joinme"].asBool()==true))
    {
        SendOffer (sender, sessionID, number, channel);
        return ;
    }

    if ((packet.isMember("type")==true) && (packet["type"].asString() == "answer"))
    {
        ProcessAnswer (sender, sessionID, number, channel, packet);
        return ;
    }

    if (packet.isMember("candidate")==true)
    {
        ProcessCandidate (sender, sessionID, number, channel, packet);
        return ;
    }

    if ((packet.isMember("hangup")==true) && (packet["hangup"].asBool()==true))
    {
        Hangup (sender, sessionID, number, channel);
        return ;
    }
}


uavos::STRUCT_SESSION_INFO* uavos::CWEBRTC_Plugin::findSessionInfoBySessionID (const char* sessionID)
{
    std::map<std::string, STRUCT_SESSION_INFO>::iterator it = m_SessionMap.begin();
 
	// Iterate over the map using Iterator till end.
	while (it != m_SessionMap.end())
	{
		// Accessing KEY from element pointed by it.
		std::string word = it->first;
        if (word.compare(sessionID)==0)
        {
            return &(it->second);
        }
		
		++it;
	}
    return nullptr;
}


bool uavos::CWEBRTC_Plugin::eraseSessionInfoBySessionID (const char* sessionID)
{
    std::map<std::string, STRUCT_SESSION_INFO>::iterator it = m_SessionMap.begin();
 
	// Iterate over the map using Iterator till end.
	while (it != m_SessionMap.end())
	{
		// Accessing KEY from element pointed by it.
		std::string word = it->first;
        if (word.compare(sessionID)==0)
        {
            m_SessionMap.erase(it);
            return true;
        }
		
		++it;
	}
    return false;
}


uavos::stream_webrtc::STRUCT_DEVICE_INFO uavos::CWEBRTC_Plugin::findDeviceInfoByLocalName (const std::string& localName)
{
    
    for(std::vector<uavos::stream_webrtc::STRUCT_DEVICE_INFO>::iterator it = m_videoDeviceInfoList.begin(); it != m_videoDeviceInfoList.end(); ++it) 
    {
        uavos::stream_webrtc::STRUCT_DEVICE_INFO deviceInfo = *it;
        if (localName.empty())
        {   // return first available camera if no camera is specified.
            return deviceInfo;
        }
        if (deviceInfo.unique_name.compare(localName.c_str()) == 0 )
        {
            std::cout << __FULL_DEBUG__ << deviceInfo.unique_name << std::endl;
            return deviceInfo;
        }
    }
    

    uavos::stream_webrtc::STRUCT_DEVICE_INFO deviceInfo_empty;
    deviceInfo_empty.device_num = -1;
    return deviceInfo_empty;

}


/**
 * Updates Array holding device info with an updated copy of a single device.
 * Location is accessed by camera guid [unique_name]. 
 **/
void uavos::CWEBRTC_Plugin::updateDeviceInfoByLocalName (const char* localName, const uavos::stream_webrtc::STRUCT_DEVICE_INFO &deviceInfo)
{
    
    for(std::vector<uavos::stream_webrtc::STRUCT_DEVICE_INFO>::iterator it = m_videoDeviceInfoList.begin(); it != m_videoDeviceInfoList.end(); ++it) 
    {
        if (it->unique_name.compare(localName) == 0 )
        {
            std::cout << __FULL_DEBUG__ << deviceInfo.unique_name << std::endl;
            it->selected   = deviceInfo.selected;
            it->active     = deviceInfo.active;
            it->recording  = deviceInfo.recording;
            it->recordFileTimeStamp = deviceInfo.recordFileTimeStamp;

            return ;
        }
    }
    
    return ;

}

void uavos::CWEBRTC_Plugin::processVideoRecording (const Json::Value &jMsg)
{
    // extract command
    
    const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    const int subCommand = cmd["C"].asInt();
    
    // double check cmd.
    if (subCommand != RemoteCommand_RECORDVIDEO) return ;
    
    bool startIfTrue = cmd["Act"].asBool();
    std::string channelName = cmd["T"].asString();
    
    if (startIfTrue == true)
    {
        startVideoRecording (jMsg);
    }
    else
    {
        stopVideoRecording (jMsg);
    }
    
}

void uavos::CWEBRTC_Plugin::stopVideoRecording (const Json::Value &jMsg)
{
    // extract command
    CWEBRTC_Plugin * cWEBRTC_Plugin;
    cWEBRTC_Plugin = &CWEBRTC_Plugin::getInstance(); 

    const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    
    std::string channelName = cmd["T"].asString();
    
    uavos::stream_webrtc::STRUCT_DEVICE_INFO device_info = cWEBRTC_Plugin->findDeviceInfoByLocalName(channelName.c_str());
    if (device_info.device_num == -1)
    {
        
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: Camera " << channelName.c_str() << " Not Found" << _NORMAL_CONSOLE_TEXT_ << std::endl;
  
        return;
    }

    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "DEBUG: Camera Found " << channelName.c_str() << _NORMAL_CONSOLE_TEXT_ << std::endl;


    #ifdef DEBUG
    std::cout << __FULL_DEBUG__  << _SUCCESS_CONSOLE_BOLD_TEXT_ << "startVideoRecording" << channelName.c_str() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    const bool res = device_info.capturer.get()->stopRecording();  

    device_info.recording = !res;
    updateDeviceInfoByLocalName(channelName.c_str(), device_info);
    
}


/**
 *  Start video recording.
 *  The logic behind video reecording is callig bash script to record video using whatever tool such as 
 *  ffmpeg or gst-launch.
 *  the script should be able to stop itself if it was running.
 * 
 **/
void uavos::CWEBRTC_Plugin::startVideoRecording (const Json::Value &jMsg)
{
    // extract command
    CWEBRTC_Plugin * cWEBRTC_Plugin;
    cWEBRTC_Plugin = &CWEBRTC_Plugin::getInstance(); 

    const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    
    const std::string channelName = cmd["T"].asString();
    

    uavos::stream_webrtc::STRUCT_DEVICE_INFO device_info = cWEBRTC_Plugin->findDeviceInfoByLocalName(channelName.c_str());
    if (device_info.device_num == -1)
    {
        
        std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: Camera " << channelName.c_str() << " Not Found" << _NORMAL_CONSOLE_TEXT_ << std::endl;
  
        return;
    }
    
    if (!device_info.capturer->isCapturing())
    {
        device_info.capturer->StartCapture();
    }
    std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "DEBUG: Camera Found " << channelName.c_str() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    const bool res = device_info.capturer.get()->startRecording();
    device_info.recordFileTimeStamp = uavos::util::CHelper::getFileTimeStamp();
    device_info.recording = res;
    updateDeviceInfoByLocalName(channelName.c_str(), device_info);
    
    
    #ifdef DEBUG
    //std::cout << __FULL_DEBUG__  << _SUCCESS_CONSOLE_BOLD_TEXT_ << "RecordCMD:"  << recCommand.str() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    
    
    
    
    return;
    
}


void uavos::CWEBRTC_Plugin::startImageCapturing (const Json::Value &jMsg)
{
    // extract command
    CWEBRTC_Plugin * cWEBRTC_Plugin;
    cWEBRTC_Plugin = &CWEBRTC_Plugin::getInstance(); 

    const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    std::string channelName = cmd["a"].asString(); // empty ""  means first available camera 
    const int numberOfImages = cmd["b"].asInt();
    const int timeBetweenShots = cmd["c"].asInt();
    //const int distanceBetweenShots = cmd["d"].asInt();  // NOT SUPPORTED
    std::cout << __FULL_DEBUG__  << "Channel:" << channelName << std::endl;
    
    uavos::stream_webrtc::STRUCT_DEVICE_INFO device_info = cWEBRTC_Plugin->findDeviceInfoByLocalName(channelName.c_str());
    if (device_info.device_num == -1)
    {
        
        #ifdef DEBUG
            std::cout << __FULL_DEBUG__  <<  _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: Camera " << channelName.c_str() << " Not Found" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif

        return;
    }

    #ifdef DEBUG
            std::cout << __FULL_DEBUG__  << std::to_string(numberOfImages)  << ":" << std::to_string(timeBetweenShots) << std::endl;
            std::cout << __FULL_DEBUG__  << _SUCCESS_CONSOLE_BOLD_TEXT_ << "DEBUG: Camera Found " << channelName.c_str() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    if (!device_info.capturer->isCapturing())
    {
        device_info.capturer->StartCapture();
    }
    device_info.capturer.get()->takeImage(numberOfImages,timeBetweenShots, this);
    device_info.recordFileTimeStamp = uavos::util::CHelper::getFileTimeStamp();
    updateDeviceInfoByLocalName(channelName.c_str(), device_info);
    
    
    #ifdef DEBUG
    //std::cout << __FULL_DEBUG__  << _SUCCESS_CONSOLE_BOLD_TEXT_ << "RecordCMD:"  << recCommand.str() << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    return;
}

/**
 * Generates JSON object array of available devices
 **/
Json::Value uavos::CWEBRTC_Plugin::getDeviceListAsJSON ()
{
    Json::Value jsonDeviceList;
    
    if (filled == false)
    {
        return jsonDeviceList;
    }


    for(std::vector<uavos::stream_webrtc::STRUCT_DEVICE_INFO>::iterator it = m_videoDeviceInfoList.begin(); it != m_videoDeviceInfoList.end(); ++it) {
    
        const uavos::stream_webrtc::STRUCT_DEVICE_INFO deviceInfo = *it;
        if (deviceInfo.selected == false)
        {
            continue;
        }

        Json::Value jsonVideoSource;

        jsonVideoSource["v"]        = (bool) true;
        jsonVideoSource["ln"]       = deviceInfo.local_name;
        jsonVideoSource["id"]       = deviceInfo.unique_name;
        jsonVideoSource["active"]   = deviceInfo.active;
        jsonVideoSource["r"]        = deviceInfo.recording;
        jsonVideoSource["p"] = EXTERNAL_CAMERA_TYPE_RTCWEBCAM;
        
        jsonDeviceList.append(jsonVideoSource);
    
        #ifdef DEBUG
        std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "getDeviceListAsJSON " << deviceInfo.recording <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
        #endif
                        
    }

    return jsonDeviceList;
}


void uavos::CWEBRTC_Plugin::onImageRecorded(std::string output_file_name, bool send_image_gcs)
{
    #ifdef DEBUG
    std::cout << _LOG_CONSOLE_TEXT << "DEBUG: onImageRecorded: " << output_file_name << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    if (!m_sendSignalJMSG) return ;
    
    if (send_image_gcs)
    {
        std::ifstream rf(output_file_name, std::ios::out | std::ios::binary);
        rf.seekg(0, std::ios::end);
        size_t size = rf.tellg();
        //char * buffer_ptr = new char[size];
        //std::unique_ptr buffer= std::unique_ptr<char[]> (buffer_ptr);
        std::unique_ptr<char[]> buffer(new char[size]);
        rf.seekg(0);
        if(rf.read(buffer.get(), size))
            std::cout << "success"<< '\n';

        std::cout << _LOG_CONSOLE_TEXT << "DEBUG: onImageRecorded: " << output_file_name << ":" << std::to_string(size) << _NORMAL_CONSOLE_TEXT_ << std::endl;
        
        m_sendBMSG ("",buffer.get(),size,TYPE_AndruavMessage_IMG, true);
        buffer.release();
    }
    else
    {
        m_sendBMSG ("",nullptr,0,TYPE_AndruavMessage_IMG, true);
    }
    
}
void uavos::CWEBRTC_Plugin::onVideoStarted()
{

}
void uavos::CWEBRTC_Plugin::onVideoStopped()
{

}