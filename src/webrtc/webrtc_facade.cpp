
#include "webrtc_facade.hpp"



using namespace de::fcb;

void CFCBFacade::Hangup(const std::string& senderPartyID, const std::string& PartyID, const std::string& channel)
{
    Json_de packet;
    packet["packet"]  = Json_de();
    packet["packet"]["hangup"] = true;
    packet["number"]  = PartyID; 
    packet["channel"] = channel;
    Json_de w;
    w["w"] = packet;
    
    m_module.sendJMSG(senderPartyID, w, TYPE_AndruavMessage_Signaling, false);
}


void CFCBFacade::OnIceCandidate (const de::STRUCT_SESSION_INFO sessionInfo, const std::string& PartyID,const webrtc::IceCandidateInterface* const candidate)
{
    std::string sdp;
    std::string sdp_mid;
    int sdp_mline_index  = 0;
    
    Json_de packet;
    packet["packet"]  = Json_de();
    
    
    if (candidate != nullptr)
    {
        candidate->ToString(&sdp);
        sdp_mid = candidate->sdp_mid();
        sdp_mline_index = candidate->sdp_mline_index();
        packet["packet"][de::stream_webrtc::kCandidateSdpName] = sdp;
    
    }
    
    
    packet["packet"][de::stream_webrtc::kCandidateSdpMidName] = sdp_mid;
    packet["packet"][de::stream_webrtc::kCandidateSdpMlineIndexName] = sdp_mline_index;
    packet["number"]  = PartyID; 
    packet["channel"] = sessionInfo.channelName;
    
    Json_de w;
    w["w"] = packet;
    m_module.sendJMSG (sessionInfo.senderPartyID, w, TYPE_AndruavMessage_Signaling, false);

}
