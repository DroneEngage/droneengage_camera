
#ifndef USERMEDIA_H
#define USERMEDIA_H

namespace de
{
namespace stream_webrtc
{

class CUserMedia  : public webrtc::RefCountInterface
{

    public:
        CUserMedia();
        ~CUserMedia();

        
    public:
        static bool InitializePeerConnection ();
        bool CreateLocalMediaStream (const char * streamId);
        static void DeletePeerConnection ();
        webrtc::scoped_refptr<webrtc::VideoTrackInterface> CreateVideoTrackInterface (
            const std::string& trackLabel, 
            webrtc::VideoTrackSourceInterface* videoTrackSourceInterface);
        bool RemoveVideoTracks ();
        bool RemoveAudioTracks ();
        webrtc::scoped_refptr<webrtc::MediaStreamInterface> GetMediaStream ()
        {
            return m_stream;
        }
        
        static webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> GetPeerConnectionFactory ()
        {
            return m_peerConnectionFactory;
        }
    protected:
        int m_peerCount;
        webrtc::scoped_refptr<webrtc::MediaStreamInterface> m_stream;
        static webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> m_peerConnectionFactory;
        //TODO: change to std::map<std::string, webrtc::scoped_refptr<webrtc::VideoTrackInterface>> m_videoTracks;
        std::map<std::string, webrtc::VideoTrackInterface*> m_videoTracks;
        std::map<std::string, webrtc::AudioTrackInterface*> m_audioTracks;
    public:         
        static std::unique_ptr<webrtc::Thread> g_worker_thread;
        static std::unique_ptr<webrtc::Thread> g_signaling_thread;
        static std::unique_ptr<webrtc::Thread> g_networking_thread;
        static webrtc::Thread* thread;
        
};
}
}
#endif