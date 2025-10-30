#ifndef WEBRTC_MESSAGE_PARSER_H
#define WEBRTC_MESSAGE_PARSER_H

#include "../de_common/helpers/json_nlohmann.hpp"
using Json_de = nlohmann::json;
#include "../de_common/de_databus/de_message_parser_base.hpp"

#include "../webrtc_plugin.hpp"

namespace de
{
namespace stream_webrtc
{
class CWebRTCMessageParser: public de::comm::CAndruavMessageParserBase
    {
    public:
        static CWebRTCMessageParser& getInstance()
        {
            static CWebRTCMessageParser instance;
            return instance;
        }

        CWebRTCMessageParser(CWebRTCMessageParser const&) = delete;
        void operator=(CWebRTCMessageParser const&) = delete;

    private:
        CWebRTCMessageParser() {}

    public:
        ~CWebRTCMessageParser() {}

    protected:
        void parseRemoteExecute(Json_de &andruav_message) override;
        void parseCommand(Json_de &andruav_message, const char *full_message, const int &full_message_length, int messageType, uint32_t permission) override;

    private:
        
        CWEBRTC_Plugin &m_WEBRTC_Plugin = CWEBRTC_Plugin::getInstance();

    };
}
}

#endif

