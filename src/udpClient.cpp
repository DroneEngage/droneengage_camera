#include <cstring> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include "common.h"
#include "udpClient.hpp"

using namespace uavos;

#define MAXLINE 8192 
char buffer[MAXLINE]; 
    
CUDPClient::CUDPClient (const char * targetIP, int broadcatsPort, const char * host, int listenningPort)
{

    // pthread initialization
    m_threadSenderID =0l;
	m_thread = pthread_self(); // get pthread ID
	pthread_setschedprio(m_thread, SCHED_FIFO); // setting priority


    // Creating socket file descriptor 
    if ( (m_SocketFD = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }

    m_ModuleAddress = new (struct sockaddr_in)();
    m_CommunicatorModuleAddress = new (struct sockaddr_in)();
    memset(m_ModuleAddress, 0, sizeof(struct sockaddr_in)); 
    memset(m_CommunicatorModuleAddress, 0, sizeof(struct sockaddr_in)); 
     
    // THIS MODULE (IP - PORT) 
    m_ModuleAddress->sin_family = AF_INET; 
    m_ModuleAddress->sin_port = htons(listenningPort); 
    m_ModuleAddress->sin_addr.s_addr = inet_addr(host);//INADDR_ANY; 
    
    // Communication Server (IP - PORT) 
    m_CommunicatorModuleAddress->sin_family = AF_INET; 
    m_CommunicatorModuleAddress->sin_port = htons(broadcatsPort); 
    m_CommunicatorModuleAddress->sin_addr.s_addr = inet_addr(targetIP); 

    // Bind the socket with the server address 
    if (bind(m_SocketFD, (const struct sockaddr *)m_ModuleAddress, sizeof(struct sockaddr_in)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 

    std::cout << "UDP Listener at " << _LOG_CONSOLE_TEXT_BOLD_ << host << ":" << listenningPort << _NORMAL_CONSOLE_TEXT_ << std::endl;

    std::cout << "Expected Comm Server at " <<  _LOG_CONSOLE_TEXT_BOLD_ << targetIP << ":" <<  broadcatsPort << _NORMAL_CONSOLE_TEXT_ << std::endl;  
}

CUDPClient::~CUDPClient ()
{
    
    pthread_join(m_threadSenderID, NULL); 	// close the thread
	pthread_join(m_thread, NULL); 	// close the thread
	close(m_SocketFD); 					// close UDP socket
	delete m_ModuleAddress;
    delete m_CommunicatorModuleAddress;
    
}

void CUDPClient::init ()
{
    // call directly as we are already in a thread.
    InternalReceiverEntry();
}



void CUDPClient::InternalReceiverEntry()
{
    std::cout << "InternalReceiverEntry called" << std::endl; 
    
    struct sockaddr_in  cliaddr;
    int n;
    __socklen_t sender_address_size = sizeof (cliaddr);
    while (true)
    {
        n = recvfrom(m_SocketFD, (char *)buffer, MAXLINE,  
                MSG_WAITALL, ( struct sockaddr *) &cliaddr, &sender_address_size);
        buffer[n]=0;
        if (m_OnReceive != NULL)
        {
             m_OnReceive((const char *) buffer,n);
        } 
    }
}


/**
 * Store ID Card in JSON
 */
void CUDPClient::SetJSONID (const Json::Value jsonID)
{
    m_JsonID = jsonID;
}

void CUDPClient::SetMessageOnReceive (void (*onReceive)(const char *, int len))
{
    m_OnReceive = onReceive;

    if (m_threadSenderID == 0l)
    {
	    const bool bsend = (pthread_create(&m_threadSenderID, NULL, this->InternalSenderIDThreadEntryFunc, (void *) this) == 0);

        if (!bsend)
        {
            perror("Error creating UDP SenderID thread\n");
            exit(EXIT_FAILURE); 
        }
    }

}

/**
 * Sending ID Periodically
 **/
void CUDPClient::InternelSenderIDEntry()
{
    std::cout << "InternelSenderIDEntry called" << std::endl; 
    while (1)
    {
        SendJMSG(m_JsonID.toStyledString().c_str());
        sleep (1);
    }
}


/**
 * Starts Sender function 
 **/
void * CUDPClient::InternalSenderIDThreadEntryFunc(void * This) {
	((CUDPClient *)This)->InternelSenderIDEntry(); 
    return NULL;
}




/**
 * Sends JMSG to Communicator
 **/
void CUDPClient::SendJMSG(const std::string& jmsg)
{
    sendto(m_SocketFD, jmsg.c_str(), jmsg.size(),  
        MSG_CONFIRM, (const struct sockaddr *) m_CommunicatorModuleAddress, 
            sizeof(struct sockaddr_in)); 
}