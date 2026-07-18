#include <iostream>
#include <fstream>
#include "../de_common/helpers/helpers.hpp"
#include "../de_common/helpers/colors.hpp"
#include "../de_common/de_databus/messages.hpp"
#include "../de_common/de_databus/configFile.hpp"
#include "../de_common/de_databus/localConfigFile.hpp"

#include "webrtc_message_parser.hpp"

using Json_de = nlohmann::json;
using namespace de::stream_webrtc;

void CWebRTCMessageParser::parseRemoteExecute(Json_de &andruav_message)
{
    const Json_de cmd = andruav_message[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    const int remoteCommand = cmd["C"].get<int>();

#ifdef DEBUG
    std::cout << "cmd: " << remoteCommand << std::endl;
#endif
    switch (remoteCommand)
    {
    case RemoteCommand_RECORDVIDEO:
    {
        if (!andruav_message.contains(ANDRUAV_PROTOCOL_SENDER))
        {
            // cannot send this command as broadcast.
            return;
        }

        bool startIfTrue = cmd["Act"].get<bool>();
        std::string channelName = cmd["T"].get<std::string>();
        std::string senderPartyID = andruav_message[ANDRUAV_PROTOCOL_SENDER].get<std::string>();

        if (startIfTrue == true)
        {
            m_WEBRTC_Plugin.startVideoRecording(andruav_message);
        }
        else
        {
            m_WEBRTC_Plugin.stopVideoRecording(andruav_message);
        }

        m_WEBRTC_Plugin.sendCameraList(senderPartyID);
    }
    break;

    case RemoteCommand_STREAMVIDEO:
    {
        if (andruav_message.contains(ANDRUAV_PROTOCOL_SENDER))
        {
            std::string senderPartyID = andruav_message[ANDRUAV_PROTOCOL_SENDER].get<std::string>();
            m_WEBRTC_Plugin.sendCameraList(senderPartyID);
        }
    }
    break;

    case RemoteCommand_SWITCHCAM:
    {
        if (andruav_message.contains(ANDRUAV_PROTOCOL_SENDER))
        {
            std::string senderPartyID = andruav_message[ANDRUAV_PROTOCOL_SENDER].get<std::string>();
            m_WEBRTC_Plugin.sendCameraList(senderPartyID);
        }
    }
    break;

    case RemoteCommand_ROTATECAM:
    {
        const Json_de cmd = andruav_message[ANDRUAV_PROTOCOL_MESSAGE_CMD];
        m_WEBRTC_Plugin.rotateCameraFrame(andruav_message);
    }
    break;
    }
}

void CWebRTCMessageParser::parseCommand(Json_de &andruav_message, const char *full_message, const int &full_message_length, int messageType, uint32_t permission)
{
    const Json_de cmd = andruav_message[ANDRUAV_PROTOCOL_MESSAGE_CMD];

    switch (messageType)
    {
    case TYPE_AndruavModule_Location_Info:
    {
        m_WEBRTC_Plugin.updateLocationInfo(andruav_message);
    }
    break;

    case TYPE_AndruavMessage_Ctrl_Cameras:
    {
        m_WEBRTC_Plugin.startImageCapturing(andruav_message);
        if (andruav_message.contains(ANDRUAV_PROTOCOL_SENDER))
        {
            std::string senderPartyID = andruav_message[ANDRUAV_PROTOCOL_SENDER].get<std::string>();
            m_WEBRTC_Plugin.sendCameraList(senderPartyID);
        }
    }
    break;

    case TYPE_AndruavMessage_Signaling:
    {

        if (!cmd.contains("w"))
            return;

        const Json_de w = cmd["w"];

        if (!w.contains("packet"))
            return;

        Json_de const packet = w["packet"];

        if (!andruav_message.contains(ANDRUAV_PROTOCOL_SENDER))
        {
            // cannot send this command as broadcast.
            return;
        }

        m_WEBRTC_Plugin.ExecuteSignalCommand(andruav_message);
        if ((packet.contains("joinme") == true) || (packet.contains("hangup") == true))
        {
            std::string senderPartyID = andruav_message[ANDRUAV_PROTOCOL_SENDER].get<std::string>();
            m_WEBRTC_Plugin.sendCameraList(senderPartyID);
        }
    }
    break;
    }
}
