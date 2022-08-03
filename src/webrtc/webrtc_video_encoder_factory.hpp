#ifndef CVIDEOENCODERFACTORY_H
#define CVIDEOENCODERFACTORY_H


namespace uavos
{
namespace stream_webrtc
{
/**
 * @brief Handle encoders supported by the system.
 * @details This function mainly uses config file "codecs" to support selective types of codes.
 * 
 */
class CBuiltinVideoEncoderFactory : public webrtc::VideoEncoderFactory {
    public:
            static std::vector<webrtc::SdpVideoFormat> SupportedFormats();
            std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;
            std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(const webrtc::SdpVideoFormat& format) override;
            

    private:
             std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder_(const webrtc::SdpVideoFormat& format);

    private:

             static void Add_VP8 (std::vector<webrtc::SdpVideoFormat> &supported_codecs);
             static void Add_VP9 (std::vector<webrtc::SdpVideoFormat> &supported_codecs);
             static void Add_H264 (std::vector<webrtc::SdpVideoFormat> &supported_codecs);
             static void Add_AX1 (std::vector<webrtc::SdpVideoFormat> &supported_codecs);
};
}
}
#endif