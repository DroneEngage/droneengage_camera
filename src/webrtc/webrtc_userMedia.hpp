
#ifndef USERMEDIA_H
#define USERMEDIA_H

namespace de
{
namespace stream_webrtc
{

class CUserMedia  : public rtc::RefCountInterface
{

    public:
        CUserMedia();
        ~CUserMedia();

        
    public:
        static bool InitializePeerConnection ();
        bool CreateLocalMediaStream (const char * streamId);
        static void DeletePeerConnection ();
        rtc::scoped_refptr<webrtc::VideoTrackInterface> CreateVideoTrackInterface (
            const std::string& trackLabel, 
            webrtc::VideoTrackSourceInterface* videoTrackSourceInterface);
        bool AddVideoTrack (webrtc::VideoTrackInterface * videoTrackInterface);
        bool RemoveVideoTracks ();
        bool RemoveAudioTracks ();
        rtc::scoped_refptr<webrtc::MediaStreamInterface> GetMediaStream ();
        
        static rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> GetPeerConnectionFactory ();
    protected:
        int m_peerCount;
        rtc::scoped_refptr<webrtc::MediaStreamInterface> m_stream;
        static rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> m_peerConnectionFactory;
        std::map<std::string, webrtc::VideoTrackInterface*> m_videoTracks;
        std::map<std::string, webrtc::AudioTrackInterface*> m_audioTracks;
    public:         
        static std::unique_ptr<rtc::Thread> g_worker_thread;
        static std::unique_ptr<rtc::Thread> g_signaling_thread;
        static std::unique_ptr<rtc::Thread> g_networking_thread;
        static rtc::Thread* thread;
        
};
}
}
#endif