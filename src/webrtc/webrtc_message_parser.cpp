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
    
}


void CWebRTCMessageParser::parseCommand(Json_de &andruav_message, const char *full_message, const int &full_message_length, int messageType, uint32_t permission)
{

}
