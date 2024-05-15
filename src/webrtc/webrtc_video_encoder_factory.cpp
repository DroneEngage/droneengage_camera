#include "../common.h"
#include "api/video_codecs/sdp_video_format.h"
#include "media/base/codec.h"
#include "media/base/media_constants.h"
#include "modules/video_coding/codecs/av1/libaom_av1_encoder.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"


#include "webrtc_video_encoder_factory.hpp"


std::vector<webrtc::SdpVideoFormat> de::stream_webrtc::CBuiltinVideoEncoderFactory::SupportedFormats() {

  #ifdef DEBUG
  std::cout << __FILE__ << "." << __FUNCTION__ << __LINE__ << _LOG_CONSOLE_BOLD_TEXT << " DEBUG: CBuiltinVideoEncoderFactory::SupportedFormats()" << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif
  std::vector<webrtc::SdpVideoFormat> supported_codecs;
  CConfigFile& cConfigFile = CConfigFile::getInstance();
  Json_de jsonConfig = cConfigFile.GetConfigJSON();
          

  if (jsonConfig.contains("codecs"))
  {
      Json_de codecs= jsonConfig["codecs"];
      for (auto codec : codecs)
      {
        if (!strcmp(de::util::CHelper::stringToUpper(codec.get<std::string>()).c_str(), "VP8"))
        {
          Add_VP8(supported_codecs);
          continue;
        }

        if (!strcmp(de::util::CHelper::stringToUpper(codec.get<std::string>()).c_str(), "VP9"))
        {
          Add_VP9 (supported_codecs);
          continue;
        }

        if (!strcmp(de::util::CHelper::stringToUpper(codec.get<std::string>()).c_str(), "H264"))
        {
          Add_H264 (supported_codecs);
          continue;
        }

        if (!strcmp(de::util::CHelper::stringToUpper(codec.get<std::string>()).c_str(), "AX1"))
        {
          Add_AX1 (supported_codecs);
          continue;
        }
      }
  }
  else
  {
    Add_VP8 (supported_codecs);
    Add_VP9 (supported_codecs);
    Add_H264 (supported_codecs);
    Add_AX1 (supported_codecs);
  }
  return supported_codecs;
}


void de::stream_webrtc::CBuiltinVideoEncoderFactory::Add_VP8 (std::vector<webrtc::SdpVideoFormat> &supported_codecs)
{
    std::cout << _INFO_CONSOLE_TEXT << "VP8" << _SUCCESS_CONSOLE_BOLD_TEXT_ << " Selected" << _NORMAL_CONSOLE_TEXT_ << std::endl; 

    supported_codecs.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
}

void de::stream_webrtc::CBuiltinVideoEncoderFactory::Add_VP9 (std::vector<webrtc::SdpVideoFormat> &supported_codecs)
{
  for (const webrtc::SdpVideoFormat& format : webrtc::SupportedVP9Codecs())
  {
    std::cout << _INFO_CONSOLE_TEXT << format.name << _LOG_CONSOLE_BOLD_TEXT <<  " Profile-id : " << format.parameters.find("profile-id")->second 
      << _SUCCESS_CONSOLE_BOLD_TEXT_ << " Selected" << _NORMAL_CONSOLE_TEXT_ << std::endl; 

    supported_codecs.push_back(format);
  }
}

void de::stream_webrtc::CBuiltinVideoEncoderFactory::Add_H264 (std::vector<webrtc::SdpVideoFormat> &supported_codecs)
{
  CConfigFile &cConfigFile = CConfigFile::getInstance();
  Json_de jsonConfig = cConfigFile.GetConfigJSON();

  for (const webrtc::SdpVideoFormat& format : webrtc::SupportedH264Codecs())
  {
    
    std::cout << _INFO_CONSOLE_TEXT << format.name << _LOG_CONSOLE_BOLD_TEXT <<  " Profile : " << format.parameters.find("profile-level-id")->second 
          << " level-asymmetry-allowed:" << format.parameters.find("level-asymmetry-allowed")->second 
          << " packetization-mode:" << format.parameters.find("packetization-mode")->second;

    // select profile level [42e01f] for iphone 
    // check this https://stackoverflow.com/questions/47462247/webrtc-is-not-working-connecting-safari-with-chrome-for-android?rq=1
    if (format.parameters.find("profile-level-id")->second.compare(std::string("42e01f")) ==0) 
    {
      std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_<< " Selected" << _NORMAL_CONSOLE_TEXT_ << std::endl;
      supported_codecs.push_back(format);
    }
    else
    {
      std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << " Skipped" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    }
  }
}

void de::stream_webrtc::CBuiltinVideoEncoderFactory::Add_AX1 (std::vector<webrtc::SdpVideoFormat> &supported_codecs)
{
  
    std::cout << _INFO_CONSOLE_TEXT << "AX1" << _SUCCESS_CONSOLE_BOLD_TEXT_ << " Selected" << _NORMAL_CONSOLE_TEXT_ << std::endl; 

    supported_codecs.push_back(webrtc::SdpVideoFormat(cricket::kAv1CodecName));
}

std::vector<webrtc::SdpVideoFormat> de::stream_webrtc::CBuiltinVideoEncoderFactory::GetSupportedFormats()
    const {

  #ifdef DEBUG
  std::cout << __FILE__ << "." << __FUNCTION__ << __LINE__ << _LOG_CONSOLE_BOLD_TEXT << " DEBUG: CBuiltinVideoEncoderFactory::GetSupportedFormats()" << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif

  return SupportedFormats();
}

std::unique_ptr<webrtc::VideoEncoder> de::stream_webrtc::CBuiltinVideoEncoderFactory::CreateVideoEncoder(
    const webrtc::SdpVideoFormat& format) {

  #ifdef DEBUG
  std::cout << __FILE__ << "." << __FUNCTION__ << __LINE__ << _LOG_CONSOLE_BOLD_TEXT << " DEBUG: CBuiltinVideoEncoderFactory::CreateVideoEncoder()" << _NORMAL_CONSOLE_TEXT_ << std::endl;
  #endif
  
  CConfigFile &cConfigFile = CConfigFile::getInstance();
  Json_de jsonConfig = cConfigFile.GetConfigJSON();
  
  std::cout << format.name << std::endl;
  if (jsonConfig.contains("codecs"))
  {
      Json_de codecs= jsonConfig["codecs"];
      for (auto codec : codecs)
      {
        std::cout << codec << std::endl;
        //TODO: fix this
        if (!strcmp(de::util::CHelper::stringToUpper(format.name).c_str(),codec.get<std::string>().c_str()))
        {
            return CreateVideoEncoder_ (format);
        }
      }
  }
  
  return CreateVideoEncoder_ (format);
}



std::unique_ptr<webrtc::VideoEncoder> de::stream_webrtc::CBuiltinVideoEncoderFactory::CreateVideoEncoder_(const webrtc::SdpVideoFormat& format) 
{
  if (!strcmp(de::util::CHelper::stringToUpper(format.name).c_str(), "VP8"))
    {
      return webrtc::VP8Encoder::Create();
    }
    if (!strcmp(de::util::CHelper::stringToUpper(format.name).c_str(), "VP9"))
    {
      return webrtc::VP9Encoder::Create(cricket::VideoCodec(format));
    }
    if (!strcmp(de::util::CHelper::stringToUpper(format.name).c_str(), "H264"))
    {
      return webrtc::H264Encoder::Create(cricket::VideoCodec(format));
    }
    //if (kIsLibaomAv1EncoderSupported &&
    if (strcmp(format.name.c_str(), cricket::kAv1CodecName))
      return webrtc::CreateLibaomAv1Encoder();
    RTC_LOG(LS_ERROR) << "Trying to created encoder of unsupported format "
                      << format.name;

    return nullptr;
}