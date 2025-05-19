#include "../common.h"



void de::stream_webrtc::CSetSessionDescriptionObserver::OnSuccess()
{
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: CSetSessionDescriptionObserver::OnSuccess" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    RTC_LOG(LS_INFO) << __FUNCTION__;
    if (!no)
    {
        m_csdocallback->onsuccess();
    }

    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}


void de::stream_webrtc::CSetSessionDescriptionObserver::OnFailure(webrtc::RTCError error)
{
    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
                  << error.message();

    m_csdocallback->onfailed(); 

    #ifdef DDEBUG
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
}
