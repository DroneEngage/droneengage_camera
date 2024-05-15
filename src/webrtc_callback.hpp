#ifndef WEBRTC_CALLBACK_H
#define WEBRTC_CALLBACK_H

namespace de
{


class CSDOCallBack
{
    public:
        virtual void onsuccess () {}
        virtual void onfailed () {}
        virtual ~CSDOCallBack() {}
};


class CCallbacks
{
    public:
        virtual ~CCallbacks() = default;

        virtual void OnLocalSdpReadytoSend (const char* sessionID, const char* type, const char* sdp) {}
        virtual void OnIceCandidate(const std::string& sessionID, const webrtc::IceCandidateInterface* candidate) {}
        virtual void OnIceConnectionDisconnected(const std::string& sessionID) {}

};



}


#endif