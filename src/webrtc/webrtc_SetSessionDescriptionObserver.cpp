#include "../common.h"



void de::stream_webrtc::CSetSessionDescriptionObserver::OnSuccess()
{
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << " " << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: CSetSessionDescriptionObserver::OnSuccess" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    RTC_LOG(INFO) << __FUNCTION__;
    if (!no)
    {
    m_csdocallback->onsuccess();
    }
}


void de::stream_webrtc::CSetSessionDescriptionObserver::OnFailure(webrtc::RTCError error)
{
    std::cout <<__FILE__ << "." << __FUNCTION__ << " line:" << __LINE__ << "\033[1;31m" << "DEBUG: CSetSessionDescriptionObserver::OnFailure" << "\033[0m" << std::endl;
    RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
                  << error.message();

    m_csdocallback->onfailed(); 
}
