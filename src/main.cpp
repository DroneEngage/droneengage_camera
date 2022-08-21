#include <stdio.h>
#include <getopt.h>
#include "common.h"
#include "udpClient.hpp"
#include "webrtc_plugin.hpp"

#include "getopt_cpp.hpp"

#include "version.h"

using namespace uavos;


std::time_t instance_time_stamp;

// UAVOS Current PartyID read from communicator
std::string  PartyID;
// UAVOS Current GroupID read from communicator
std::string  GroupID;
std::string  ModuleID;
std::string  ModuleKey;

CConfigFile *cConfigFile;
uavos::comm::CUDPClient& cUDPClient = uavos::comm::CUDPClient::getInstance();  
CWEBRTC_Plugin * cWEBRTC_Plugin;

const std::string createJSONID (bool reSend);
void onReceive (const char * jsonMessage, int len);
pthread_t createUDPSocket_thread;

static std::string configName = "config.module.json";


/**
 * @brief display version info
 * 
 */
void _version (void)
{
    std::cout << std::endl << _SUCCESS_CONSOLE_BOLD_TEXT_ "Drone-Engage Camera Plugin version " << _INFO_CONSOLE_TEXT << version_string << _NORMAL_CONSOLE_TEXT_ << std::endl;
}


/**
 * @brief display help for -h command argument.
 * 
 */
void _usage(void)
{
    _version ();
    std::cout << std::endl << _INFO_CONSOLE_TEXT "Options" << _NORMAL_CONSOLE_TEXT_ << std::ends;
    std::cout << std::endl << _INFO_CONSOLE_TEXT "\t--config:      -c ./config.json   default [./config.module.json]" << _NORMAL_CONSOLE_TEXT_ << std::ends;
    std::cout << std::endl << _INFO_CONSOLE_TEXT "\t--version:     -v" << _NORMAL_CONSOLE_TEXT_ << std::endl;
}


// void * createUDPSocket (void * cUDPClient)
// {
//     (cUDPClient.start();
    
//     // if ((cUDPClient.StartInternalThread())
//     // {
//     //     std::cout << "UDP Server Started" << std::endl;
//     // }

//     return NULL;
// }

/**
 * @brief 
 * 
 * @param targetID  target ID could be empty string if routingType is broadcast.
 * @param jmsg  message contents
 * @param routingType communication type [peer-to-peer or broadcast] 
 * @param commandType  Inter module communication or what ?
 * @param andruav_message_id  message type ID
 * @param internal_message true if message should be handled by communication_module
 */
void sendJMSG (
    const std::string& targetPartyID,
    const Json::Value& jmsg,
    const int& andruav_message_id,
    const bool& internal_message = false)
{
    Json::Value fullMessage;

    // default routing is CMD_COMM_GROUP i.e. forward to andruav_server and send to all.
    std::string msgRoutingType = CMD_COMM_GROUP;
    if (internal_message == true)
    {
        // this is an internal message, should be processed and/or resent by communication module to other modules.
        msgRoutingType = CMD_TYPE_INTERMODULE;
        fullMessage[INTERMODULE_MODULE_KEY]         = ModuleKey;
    }
    else
    {
        if (targetPartyID.length() != 0 )
        {
            msgRoutingType = CMD_COMM_INDIVIDUAL;
        }   
    }
    
    fullMessage[ANDRUAV_PROTOCOL_TARGET_ID]         = targetPartyID; // targetID can exist even if routing is intermodule
    fullMessage[INTERMODULE_ROUTING_TYPE]           = msgRoutingType;
    fullMessage[ANDRUAV_PROTOCOL_MESSAGE_TYPE]      = andruav_message_id;
    fullMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD]       = jmsg;
    
    const std::string& msg = fullMessage.toStyledString();
    cUDPClient.sendMSG(msg.c_str(), msg.length());
}


/**
 * @brief sends binary packet
 * @details sends binary packet.
 * Binary packet always has JSON header then 0 then binary data.
 * 
 * @param targetPartyID 
 * @param bmsg 
 * @param andruav_message_id 
 * @param internal_message if true @link INTERMODULE_MODULE_KEY @endlink equaqls to Module key
 */
void sendBMSG (
    const std::string& targetPartyID, 
    const char * bmsg, 
    const int bmsg_length, 
    const int& andruav_message_id, 
    const bool& internal_message = false)
{
    Json::Value fullMessage;

    std::string msgRoutingType = CMD_COMM_GROUP;
    if (internal_message == true)
    {
        msgRoutingType = CMD_TYPE_INTERMODULE;
        fullMessage[INTERMODULE_MODULE_KEY]             = ModuleKey;
    }
    else
    {
        if (targetPartyID.length() != 0 )
        {
                msgRoutingType = CMD_COMM_INDIVIDUAL;
        }
    }
    
    fullMessage[ANDRUAV_PROTOCOL_TARGET_ID]             = targetPartyID; // targetID can exist even if routing is intermodule
    fullMessage[INTERMODULE_ROUTING_TYPE]               = std::string(msgRoutingType);
    fullMessage[ANDRUAV_PROTOCOL_MESSAGE_TYPE]          = andruav_message_id;
    std::string json_msg = fullMessage.toStyledString();
    
    // prepare an array for the whole message
    char * msg_ptr = new char[json_msg.length() + 1 + bmsg_length];
    std::unique_ptr<char []> msg = std::unique_ptr<char []> (msg_ptr);
    // copy json part
    strcpy(msg_ptr,json_msg.c_str());
    // add zero '0' delimeter
    msg_ptr[json_msg.length()] = 0;
    // copy binary message
    if (bmsg_length != 0)
    {
        // empty binary contents of a binary can exist if binary contents is optional
        // or will be filled by communicator module.
        memcpy(&msg[json_msg.length()+1], bmsg, bmsg_length);
    }

    cUDPClient.sendMSG(msg_ptr, json_msg.length()+1+bmsg_length);
    
    msg.release();
}

/**
 * @brief Sends TYPE_AndruavMessage_Signaling message
 * 
 * @param senderPartyID 
 * @param jmsg 
 */
void sendSignallingJMSG (const char * senderPartyID, const Json::Value& jmsg)
{
    
    Json::Value webrtcMsg;
    sendJMSG (senderPartyID, jmsg,
        TYPE_AndruavMessage_Signaling, false);
}


void initArguments (int argc, char *argv[])
{
    int opt;
    const struct GetOptLong::option options[] = {
    {"config",     true,   0, 'c'},
    {"version",    false,  0, 'v'},
    {"help",       false,  0, 'h'},
    {0, false, 0, 0}
    };
    // adding ':' means there is extra parameter needed
    GetOptLong gopt(argc, argv, "c:vh",
            options);

    /*
      parse command line options
     */
    while ((opt = gopt.getoption()) != -1) {
    switch (opt) {
    case 'c':
        configName = gopt.optarg;
        break;
    case 'v':
        _version();
        exit(0);
        break;
    case 'h':
        _usage();
        exit(0);
    default:
        printf("Unknown option '%c'\n", (char)opt);
        exit(1);
    }
    }
}

/**
 * initialize components
 **/
void init (int argc, char *argv[]) 
{
    instance_time_stamp = std::time(nullptr);
    
    initArguments (argc, argv);

    // Reading Configuration
    std::cout << std::endl << "=================== " << "STARTING PLUGIN ===================" << std::endl;
    _version();


    cConfigFile = &CConfigFile::getInstance();
    cConfigFile->InitConfigFile (configName.c_str());
    Json::Value& jsonConfig = cConfigFile->GetConfigJSON();
    
    ModuleID = jsonConfig["module_id"].asString();
    ModuleKey = jsonConfig["module_key"].asString();
    std::string  ModuleKey;

    //https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
    std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "UAVOS Plugin Module: " << _SUCCESS_CONSOLE_BOLD_TEXT_ <<  ModuleID << _NORMAL_CONSOLE_TEXT_ << std::endl;
    std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "Class Type: " << _SUCCESS_CONSOLE_BOLD_TEXT_<< "camera" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    std::cout << std::asctime(std::localtime(&instance_time_stamp)) << instance_time_stamp << " seconds since the Epoch" << std::endl;
    
    // INIT WEBRTC
    cWEBRTC_Plugin = &CWEBRTC_Plugin::getInstance(); 
    
    cWEBRTC_Plugin->RegisterSendSignalJMSG(sendSignallingJMSG);
    cWEBRTC_Plugin->RegisterSendJMSG(sendJMSG);
    cWEBRTC_Plugin->RegisterSendBMSG(sendBMSG);
    
    const bool singleCameraMode = jsonConfig.isMember("one_session_per_camera")?jsonConfig["one_session_per_camera"].asBool():true;
    cWEBRTC_Plugin->initCameras(singleCameraMode);
    if (jsonConfig.isMember("camera_list"))
    {
        Json::Value jsonCameraList= jsonConfig["camera_list"];
        for (auto cameraItem : jsonCameraList)
        {
            if (cameraItem["name"].asString().empty()) continue; // most propably it is an extra comma after last field.
            std::cout << cameraItem["name"].asString() << "ID:" << cameraItem["device_num"].asInt() << std::endl;
            cWEBRTC_Plugin->addCameraByID(cameraItem["name"].asString(), cameraItem["device_num"].asInt());
        }
	
    }
    else
    {
        const int minCameraIndex = jsonConfig.isMember("camera_start_index")?jsonConfig["camera_start_index"].asInt():0;
        const int maxCameraIndex = jsonConfig.isMember("camera_end_index")?jsonConfig["camera_end_index"].asInt():999;
        cWEBRTC_Plugin->addCameraByRange(minCameraIndex, maxCameraIndex);
    }

    //BUG: for unknown reason calling "InitializePeerConnection" with the "cWEBRTC_Plugin->init" generates strange error.type
    // error is: if you send JSON of camera lists and the list length is one then it will corrupt  and will not talk to server.
    cWEBRTC_Plugin->InitializePeerConnection();
    
    
    // UDP Server
    cUDPClient.init (jsonConfig["s2s_udp_target_ip"].asCString(),
            std::stoi(jsonConfig["s2s_udp_target_port"].asCString()),
            jsonConfig["s2s_udp_listening_ip"].asCString() ,
            std::stoi(jsonConfig["s2s_udp_listening_port"].asCString()));
    
    cUDPClient.SetJSONID (createJSONID(true));
    cUDPClient.SetMessageOnReceive (&onReceive);
    cUDPClient.start();
}


void uninit ()
{
   delete cWEBRTC_Plugin;
}


/**
 * creates JSON message that identifies Module
 **/
const std::string createJSONID (bool reSend)
{
    Json::Reader reader;
    Json::Value MESSAGE_FILTER;
    
    
    MESSAGE_FILTER.append(TYPE_AndruavMessage_RemoteExecute);
    MESSAGE_FILTER.append(TYPE_AndruavMessage_Signaling);
    MESSAGE_FILTER.append(TYPE_AndruavMessage_Ctrl_Cameras);

    const Json::Value& jsonConfig = cConfigFile->GetConfigJSON();
    
    Json::Value jsonID;
    jsonID[INTERMODULE_ROUTING_TYPE] =  CMD_TYPE_INTERMODULE;
    jsonID[ANDRUAV_PROTOCOL_MESSAGE_TYPE] =  TYPE_AndruavModule_ID;
    Json::Value ms;
    const Json::Value cameraList = cWEBRTC_Plugin->getDeviceListAsJSON();
    ms[JSON_INTERMODULE_MODULE_ID] = jsonConfig["module_id"];
    ms[JSON_INTERMODULE_MODULE_CLASS] = "camera";
    ms[JSON_INTERMODULE_MODULE_MESSAGES_LIST]   = MESSAGE_FILTER; //Json::Value(["C","V"]); //Json::value::array(MESSAGE_FILTER);
    Json::Value list;
    list.append("C");
    list.append("V");
    ms[JSON_INTERMODULE_MODULE_FEATURES] = list; //jsonConfig["module_features"];
    ms[JSON_INTERMODULE_MODULE_KEY] = jsonConfig["module_key"];
    ms["m"] = cameraList;
    ms[JSON_INTERMODULE_RESEND] = reSend;
    ms[JSON_INTERMODULE_TIMESTAMP_INSTANCE]     = Json::Int64(instance_time_stamp);

    jsonID[ANDRUAV_PROTOCOL_MESSAGE_CMD] = ms;
            
    return jsonID.toStyledString();
}


/*
    Process received messages from UDP client.
    most propably received from UAVOS_Communicator
*/
void onReceive (const char * jsonMessage, int len)
{
    static bool bFirstReceived = false;

    Json::Reader reader;
    Json::Value jMsg;
    
    reader.parse(jsonMessage, jMsg);

    int andruav_message_id = jMsg[ANDRUAV_PROTOCOL_MESSAGE_TYPE].asInt();
        
    if (std::strcmp(jMsg[INTERMODULE_ROUTING_TYPE].asCString(),CMD_TYPE_INTERMODULE)==0)
    {
        if (andruav_message_id == TYPE_AndruavModule_ID)
        {
            const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            const Json::Value moduleID = cmd ["f"]; // ex: "f":{"gr":"1","sd":"pad_uavos_os"}
            PartyID = std::string(moduleID[ANDRUAV_PROTOCOL_SENDER].asCString());
            GroupID = std::string(moduleID[ANDRUAV_PROTOCOL_GROUP_ID].asCString());

            if (!bFirstReceived)
            { 
                // tell server you dont need to send ID again (z=false).
                std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Communicator Server Found " <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
                cUDPClient.SetJSONID (createJSONID(false));
                bFirstReceived = true;
            }
            return ;
        }
    }

    std::cout << "andruav_message_id: " << andruav_message_id << std::endl;
    switch (andruav_message_id)
    {
        case TYPE_AndruavMessage_Ctrl_Cameras:
        {
            const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            cWEBRTC_Plugin->startImageCapturing(jMsg);
            cUDPClient.SetJSONID (createJSONID(false));
        }
        break;

        case TYPE_AndruavMessage_RemoteExecute:
        {
            const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            const int remoteCommand = cmd["C"].asInt();
            std::cout << "cmd: " << remoteCommand << std::endl;
            switch (remoteCommand)
            {
                case RemoteCommand_RECORDVIDEO:
                    std::cout << "Key " << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: RemoteCommand_RECORDVIDEO" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                    std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: TYPE_AndruavMessage_Signaling" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                    if (!jMsg.isMember(ANDRUAV_PROTOCOL_SENDER))
                    {
                        // cannot send this command as broadcast.
                        return ;                    
                    }
                    cWEBRTC_Plugin->processVideoRecording(jMsg);   
                    cUDPClient.SetJSONID (createJSONID(false));
                break;
                
                case RemoteCommand_STREAMVIDEO:
                    // const bool act = cmd["Act"].asBool();
                    // if (!act)
                    // {
                    //     // act true is not handled here.
                    //     //TODO: replace act false with hangout signalling or replicate it with another signalling message "hangout"
                    //     cWEBRTC_Plugin->
                    // }
                    cUDPClient.SetJSONID (createJSONID(false));
                break;

                case RemoteCommand_SWITCHCAM:
                    //_camera.nextCamera (jmsg.sd);
                    std::cout << "Key " << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: RemoteCommand_SWITCHCAM" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                    cUDPClient.SetJSONID (createJSONID(false));
                break;

            }
        }
        break;

        case TYPE_AndruavMessage_Signaling:
        {
            const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            const Json::Value w = cmd["w"];
    
            Json::Value const packet = w["packet"];
            
            std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "INFO: TYPE_AndruavMessage_Signaling" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            if (!jMsg.isMember(ANDRUAV_PROTOCOL_SENDER))
            {
                // cannot send this command as broadcast.
                return ;                    
            }
            //
            cWEBRTC_Plugin->ExecuteSignalCommand(jMsg);  
            if ((packet.isMember("joinme")==true) || (packet.isMember("hangup")==true))
            {
                cUDPClient.SetJSONID (createJSONID(false));
            }
        }
        break;
    } 
    
}

int main(int argc, char *argv[])
{
    init (argc, argv);

    while (1)
    {
        if (cWEBRTC_Plugin!= NULL)
        {
            cWEBRTC_Plugin->cleaning();
        }
        sleep (1);
        //sleep (1000);
    }

    //uninit();
}