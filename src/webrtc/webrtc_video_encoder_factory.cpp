#include "../common.h"
#include "api/video_codecs/sdp_video_format.h"
#include "media/base/codec.h"
#include "media/base/media_constants.h"
#include "modules/video_coding/codecs/av1/libaom_av1_encoder.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"

#include "webrtc_video_encoder_factory.hpp"


std::vector<webrtc::SdpVideoFormat> de::stream_webrtc::CBuiltinVideoEncoderFactory::GetSupportedFormats() const {
  return factory_.GetSupportedFormats();
}

std::unique_ptr<webrtc::VideoEncoder> de::stream_webrtc::CBuiltinVideoEncoderFactory::Create(
    const webrtc::Environment& env,
    const webrtc::SdpVideoFormat& format) {
  if (std::optional<webrtc::SdpVideoFormat> original_format =
          webrtc::FuzzyMatchSdpVideoFormat(factory_.GetSupportedFormats(),
                                           format))  {
    return std::make_unique<webrtc::SimulcastEncoderAdapter>(
        env, &factory_, nullptr, *original_format);
  }

  return nullptr;
}

webrtc::VideoEncoderFactory::CodecSupport de::stream_webrtc::CBuiltinVideoEncoderFactory::QueryCodecSupport(
    const webrtc::SdpVideoFormat& format,
    std::optional<std::string> scalability_mode) const {
  return factory_.QueryCodecSupport(format, scalability_mode);
}