#ifndef CUDPCLIENT_H

#define CUDPCLIENT_H
namespace uavos
{
class CUDPClient
{

    public:
        CUDPClient (const char * targetIP, int broadcatsPort, const char * host, int listenningPort);
        ~CUDPClient ();
        void init();
        void SetJSONID (const Json::Value jsonID);
        void SetMessageOnReceive (void (*onReceive)(const char *, int len));
        void SendJMSG(const std::string& jmsg);

    protected:
        // This static function only needed once
        // it sends ID to communicator. 
        // you need to create UDP with communicator first.
        static void * InternalSenderIDThreadEntryFunc(void * func);

        void InternalReceiverEntry();
        void InternelSenderIDEntry();

        struct sockaddr_in  *m_ModuleAddress, *m_CommunicatorModuleAddress; 
        int m_SocketFD; 
        pthread_t m_thread,m_threadSenderID;

        Json::Value m_JsonID;
        void (*m_OnReceive)(const char *, int len);
        
};
}

#endif