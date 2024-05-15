#ifndef CPEERCONNECTION_H
#define CPEERCONNECTION_H



namespace de
{
namespace stream_webrtc 
{


const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";


typedef struct webrtc::PeerConnectionInterface::IceServer ICE_SERVER;



class CPeerConnectionManager : public webrtc::CreateSessionDescriptionObserver 
                             , public webrtc::PeerConnectionObserver 
                             , public de::CSDOCallBack
{

    public:
        
        CPeerConnectionManager();
        explicit CPeerConnectionManager(CCallbacks *callbacks);

        ~CPeerConnectionManager();
        
        bool CreatePeerConnection(const std::string &sessionID, const std::string &peerID, const std::string &channelNumber, const std::string &channelName);
        bool AddStream(webrtc::MediaStreamInterface *stream);
        void AddTrack(rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track, const std::vector<std::string>& stream_ids);
        bool CreateOffer();
        bool CreateAnswer();
        void SetRemoteDescription(const std::string& type, const std::string& sdp);
        bool AddIceCandidate (const Json_de& packet);
        webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpTransceiverInterface>> AddTransceiver(
            rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track);
        void Close();


    public:
        // CreateSessionDescriptionObserver
        // This callback transfers the ownership of the |desc|.
        // TODO(deadbeef): Make this take an std::unique_ptr<> to avoid confusion
        // around ownership.
        void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
        // The OnFailure callback takes an RTCError, which consists of an
        // error code and a string.
        // RTCError is non-copyable, so it must be passed using std::move.
        // Earlier versions of the API used a string argument. This version
        // is deprecated; in order to let clients remove the old version, it has a
        // default implementation. If both versions are unimplemented, the
        // result will be a runtime error (stack overflow). This is intentional.
        void OnFailure(webrtc::RTCError error) override;
        

    public:
        //SetSessionDescriptionObserver
        //void OnSuccess() override;
        void onsuccess ()  override;
        void onfailed ()  override;


    public:
        //webrtc::PeerConnectionObserver
            // Triggered when the SignalingState changed.
            void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override;

            // Triggered when media is received on a new stream from remote peer.
            void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;

            // Triggered when a remote peer closes a stream.
            void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;

            // Triggered when a remote peer opens a data channel.
            void OnDataChannel(
                rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;

            // Triggered when renegotiation is needed. For example, an ICE restart
            // has begun.
            void OnRenegotiationNeeded() override;

            // Called any time the legacy IceConnectionState changes.
            //
            // Note that our ICE states lag behind the standard slightly. The most
            // notable differences include the fact that "failed" occurs after 15
            // seconds, not 30, and this actually represents a combination ICE + DTLS
            // state, so it may be "failed" if DTLS fails while ICE succeeds.
            //
            // TODO(jonasolsson): deprecate and remove this.
            virtual void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;


            // Called any time the PeerConnectionState changes.
            void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state) override;

            // Called any time the IceGatheringState changes.
            void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

            // A new ICE candidate has been gathered.
            void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;

            

            // Ice candidates have been removed.
            // TODO(honghaiz): Make this a pure virtual method when all its subclasses
            // implement it.
            void OnIceCandidatesRemoved(const std::vector<cricket::Candidate>& candidates) override;

            // Called when the ICE connection receiving status changes.
            void OnIceConnectionReceivingChange(bool receiving) override;

            
            // This is called when a receiver and its track are created.
            // TODO(zhihuang): Make this pure virtual when all subclasses implement it.
            // Note: This is called with both Plan B and Unified Plan semantics. Unified
            // Plan users should prefer OnTrack, OnAddTrack is only called as backwards
            // compatibility (and is called in the exact same situations as OnTrack).
            void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) override;

            // This is called when signaling indicates a transceiver will be receiving
            // media from the remote endpoint. This is fired during a call to
            // SetRemoteDescription. The receiving track can be accessed by:
            // |transceiver->receiver()->track()| and its associated streams by
            // |transceiver->receiver()->streams()|.
            // Note: This will only be called if Unified Plan semantics are specified.
            // This behavior is specified in section 2.2.8.2.5 of the "Set the
            // RTCSessionDescription" algorithm:
            // https://w3c.github.io/webrtc-pc/#set-description
            void OnTrack(rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;

            // Called when signaling indicates that media will no longer be received on a
            // track.
            // With Plan B semantics, the given receiver will have been removed from the
            // PeerConnection and the track muted.
            // With Unified Plan semantics, the receiver will remain but the transceiver
            // will have changed direction to either sendonly or inactive.
            // https://w3c.github.io/webrtc-pc/#process-remote-track-removal
            // TODO(hbos,deadbeef): Make pure virtual when all subclasses implement it.
            void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;

            // Called when an interesting usage is detected by WebRTC.
            // An appropriate action is to add information about the context of the
            // PeerConnection and write the event to some kind of "interesting events"
            // log function.
            // The heuristics for defining what constitutes "interesting" are
            // implementation-defined.
            void OnInterestingUsage(int usage_pattern) override;


    protected:
        //de::ICE_SERVER GetIceServerFromUrl(const std::string &url, const std::string &clientIp = "");

    private:
        
        rtc::scoped_refptr<webrtc::PeerConnectionInterface> m_peerConnection;
        //std::unique_ptr <de::CPeerConnectionObserver>  m_peerConnectionObserver;
        //std::unique_ptr <webrtc::PeerConnectionInterface::RTCConfiguration>  m_config;
        webrtc::PeerConnectionInterface::RTCConfiguration  m_config;

    private:
            CCallbacks *m_callbacks = NULL;
            std::string m_peerID;
            std::string m_channelNumber;
            std::string m_channelName;
	        std::string m_sessionID;

            
};

}
}

#endif
