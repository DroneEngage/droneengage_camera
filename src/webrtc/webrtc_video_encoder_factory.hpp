#ifndef CVIDEOENCODERFACTORY_H
#define CVIDEOENCODERFACTORY_H

namespace de {
namespace stream_webrtc {
/**
 * @brief Handle encoders supported by the system.
 * @details This function mainly uses config file "codecs" to support selective
 * types of codes.
 *
 */
class CBuiltinVideoEncoderFactory : public webrtc::VideoEncoderFactory {
 public:
  std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

  std::unique_ptr<webrtc::VideoEncoder> Create(
      const webrtc::Environment& env,
      const webrtc::SdpVideoFormat& format) override;

  webrtc::VideoEncoderFactory::CodecSupport QueryCodecSupport(
      const webrtc::SdpVideoFormat& format,
      std::optional<std::string> scalability_mode) const override;

 private:
  webrtc::VideoEncoderFactoryTemplate<webrtc::LibvpxVp8EncoderTemplateAdapter,
                                      webrtc::LibvpxVp9EncoderTemplateAdapter,
                                      webrtc::OpenH264EncoderTemplateAdapter,
                                      webrtc::LibaomAv1EncoderTemplateAdapter>
      factory_;
};
}  // namespace stream_webrtc
}  // namespace de
#endif