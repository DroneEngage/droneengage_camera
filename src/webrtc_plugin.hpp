
#ifndef CWEBRTC_PLUGIN_H
#define CWEBRTC_PLUGIN_H


#define MIN_CAMERA_INDEX 0
#define MAX_CAMERA_INDEX 999

namespace uavos
{

struct session_info {
    std::string  senderPartyID; 
    std::string  sessionID; // combination of channelName and channelNumber
    int peerObject;
    std::string channelName;   // equivelant to partyID
    std::string channelNumber; // unique camera name --> deviceID
    rtc::scoped_refptr<uavos::stream_webrtc::CPeerConnectionManager>  peerConnectionManager;
    
};

typedef struct session_info STRUCT_SESSION_INFO;

class CWEBRTC_Plugin : public CCallbacks
{


    public:
        //https://stackoverflow.com/questions/1008019/c-singleton-design-pattern
        static CWEBRTC_Plugin& getInstance()
        {
            static CWEBRTC_Plugin    instance; // Guaranteed to be destroyed.
                                            // Instantiated on first use.
            return instance;
        }
        CWEBRTC_Plugin(CWEBRTC_Plugin const&)           = delete;
        void operator=(CWEBRTC_Plugin const&)           = delete;

        // Note: Scott Meyers mentions in his Effective Modern
        //       C++ book, that deleted functions should generally
        //       be public as it results in better error messages
        //       due to the compilers behavior to check accessibility
        //       before deleted status

    public:
        void OnLocalSdpReadytoSend (const char* sessionID,const char* type, const char* sdp) override;
        void OnIceCandidate (const std::string& sessionID, const webrtc::IceCandidateInterface* const candidate) override;
        void OnIceConnectionDisconnected (const std::string& sessionID) override;

    private:
        CWEBRTC_Plugin() {}                    // Constructor? (the {} brackets) are needed here.

        // C++ 11
        // =======
        // We can use the better technique of deleting the methods
        // we don't want.
    public:
        ~CWEBRTC_Plugin ();
        void initCameras(const bool singleCameraMode);
        bool addCameraByID (std::string cameraVideoName, int cameraVideoIndex);
        void addCameraByRange (int startVideoIndex, int endVideoIndex);
        void InitializePeerConnection();
        
        Json::Value getDeviceListAsJSON ();

        void ExecuteSignalCommand(const Json::Value& cmd);

        void RegisterSendJMSG (SENDJMSG_CALLBACK sendJMSG)
        {
            m_sendJMSG = sendJMSG;
        }



    public:
        uavos::stream_webrtc::STRUCT_DEVICE_INFO findDeviceInfoByLocalName (const char * localName);
        void updateDeviceInfoByLocalName (const char* localName, const uavos::stream_webrtc::STRUCT_DEVICE_INFO &deviceInfo);
        void cleaning ();

    protected:
        void SendOffer (const std::string& senderPartyID, const std::string& sessionID, const std::string& channelNumber, const std::string& channelName);
        void ProcessAnswer (const std::string& senderPartyID, const std::string& sessionID, const std::string& channelNumber, const std::string& channelName, const Json::Value& packet);
        void ProcessCandidate (const std::string& senderPartyID, const std::string& sessionID, const std::string& channelNumber, const std::string& channelName, const Json::Value& packet);
        void Hangup (const std::string& senderPartyID, const std::string& sessionID,  const std::string& number,  const std::string& channel);

        bool IsCorrectCameraIndex (const int cameraIndex);
        // There is a problem with search using string or char * that is why 
        // I make manual search.
        STRUCT_SESSION_INFO *findSessionInfoBySessionID (const char * sessionID);
        bool eraseSessionInfoBySessionID (const char * sessionID);
        
        
    protected:
        
        bool m_singleCameraMode=true;
        int m_singleCameraModeCameraIndex = -1;
        int m_actualVideoSourcesCount = 0;
        int m_actualAudioSourcesCount = 0;
        int m_StartVideoIndex = MIN_CAMERA_INDEX;
        int m_EndVideoIndex = MAX_CAMERA_INDEX;
        
        std::map<std::string, STRUCT_SESSION_INFO> m_SessionMap;
        std::map<std::string, STRUCT_SESSION_INFO> m_localTracksMap;

        bool filled = ATOMIC_VAR_INIT(false);
        std::vector<uavos::stream_webrtc::STRUCT_DEVICE_INFO> m_videoDeviceInfoList;
        std::vector<uavos::stream_webrtc::STRUCT_DEVICE_INFO> m_audioDeviceInfoList;

        rtc::scoped_refptr <uavos::stream_webrtc::CUserMedia> m_connection ;
        SENDJMSG_CALLBACK m_sendJMSG = NULL;

        std::vector<std::string> deleteMe;
        
};

}

#endif