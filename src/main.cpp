


#include <stdio.h>
#include <getopt.h>
#include "common.h"
#include "udpClient.hpp"
#include "webrtc_plugin.hpp"

#include "getopt_cpp.hpp"

#include "version.h"

using namespace uavos;


std::time_t time_stamp;

// UAVOS Current PartyID read from communicator
std::string  PartyID;
// UAVOS Current GroupID read from communicator
std::string  GroupID;
std::string  ModuleID;

CConfigFile *cConfigFile;
CUDPClient * cUDPClient;       
CWEBRTC_Plugin * cWEBRTC_Plugin;
media_recorder::video::CVideoRecorder * cVideoRecorder;

const Json::Value createJSONID (bool reSend);
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
    std::cout << std::endl << _INFO_CONSOLE_TEXT "\t--config:          -c ./config.json   default [./config.module.json]" << _NORMAL_CONSOLE_TEXT_ << std::ends;
    std::cout << std::endl << _INFO_CONSOLE_TEXT "\t--version:         -v" << _NORMAL_CONSOLE_TEXT_ << std::endl;
}


void * createUDPSocket (void * cUDPClient)
{
        ((CUDPClient *)cUDPClient)->init();
        
        // if (((CUDPClient *)cUDPClient)->StartInternalThread())
        // {
        //         std::cout << "UDP Server Started" << std::endl;
        // }

        return NULL;
}

const std::string generateForwardSendCMD (
        // target ID could be empty string if commType is broadcast.
        const std::string& targetID,
        // communication type [peer-to-peer or broadcast] 
        const std::string& commType,
        // Inter module communication or what ?
        const std::string& commandType,
        // message type ID
        const int messageType,
        // message contents
        const Json::Value& jmsg)
{
        Json::Value fullMessage;

        fullMessage[ANDRUAV_PROTOCOL_TARGET_ID]         = std::string(targetID);
        fullMessage[INTERMODULE_COMMAND_TYPE]           = std::string(commType);
        //fullMessage[ANDRUAV_PROTOCOL_COMM_TYPE]         = commType;
        fullMessage[ANDRUAV_PROTOCOL_MESSAGE_TYPE]      = messageType;
        fullMessage[ANDRUAV_PROTOCOL_MESSAGE_CMD]       = jmsg;
        return fullMessage.toStyledString();
}

void sendJMSG (const char * senderPartyID, const Json::Value& jmsg)
{
        
        Json::Value webrtcMsg;
        std::string commType = CMD_COMM_INDIVIDUAL;
        if ((!senderPartyID) || (std::string(senderPartyID).empty()))
        {
                commType = CMD_COMM_GROUP;
        }

        const std::string fullMessage = generateForwardSendCMD (senderPartyID, commType, std::string(CMD_TYPE_INTERMODULE),
                TYPE_AndruavMessage_Signaling, jmsg);
        cUDPClient->SendJMSG(fullMessage);

}


void initArguments (int argc, char *argv[])
{
    int opt;
    const struct GetOptLong::option options[] = {
        {"config",         true,   0, 'c'},
        {"version",        false,  0, 'v'},
        {"help",           false,  0, 'h'},
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
        time_stamp = std::time(nullptr);
        
        initArguments (argc, argv);

        // Reading Configuration
        std::cout << std::endl << "=================== " << "STARTING PLUGIN ===================" << std::endl;
        _version();


        cConfigFile = &CConfigFile::getInstance();
        cConfigFile->InitConfigFile (configName.c_str());
        Json::Value& jsonConfig = cConfigFile->GetConfigJSON();
    
        ModuleID = jsonConfig["module_id"].asString();
        //https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
        std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "UAVOS Plugin Module: " << _SUCCESS_CONSOLE_BOLD_TEXT_ <<  ModuleID << _NORMAL_CONSOLE_TEXT_ << std::endl;
        std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "Class Type: " << _SUCCESS_CONSOLE_BOLD_TEXT_<< "camera" << _NORMAL_CONSOLE_TEXT_ << std::endl;

        std::cout << std::asctime(std::localtime(&time_stamp)) << time_stamp << " seconds since the Epoch" << std::endl;
        
        // INIT WEBRTC
        cWEBRTC_Plugin = &CWEBRTC_Plugin::getInstance(); 
        
        cWEBRTC_Plugin->RegisterSendJMSG(sendJMSG);
        
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
        
        
        // Video Recorder
        cVideoRecorder = &media_recorder::video::CVideoRecorder::getInstance();
        cVideoRecorder->init();

        // UDP Server
        cUDPClient = new CUDPClient (jsonConfig["s2s_udp_target_ip"].asCString(),
                        std::stoi(jsonConfig["s2s_udp_target_port"].asCString()),
                        jsonConfig["s2s_udp_listening_ip"].asCString() ,
                        std::stoi(jsonConfig["s2s_udp_listening_port"].asCString()));
        
        ((CUDPClient *)cUDPClient)->SetJSONID (createJSONID(true));
        ((CUDPClient *)cUDPClient)->SetMessageOnReceive (&onReceive);
        if(pthread_create(&createUDPSocket_thread, NULL, createUDPSocket, (void *)cUDPClient )) {
                fprintf(stderr, "Error creating thread\n");
                return ;
        }

        
        
}


void uninit ()
{
        delete cUDPClient;
        delete cWEBRTC_Plugin;
}


/**
 * creates JSON message that identifies Module
 **/
const Json::Value createJSONID (bool reSend)
{
        Json::Reader reader;
        Json::Value MESSAGE_FILTER;
        
        
        MESSAGE_FILTER.append(TYPE_AndruavMessage_RemoteExecute);
        MESSAGE_FILTER.append(TYPE_AndruavMessage_Signaling);
        MESSAGE_FILTER.append(TYPE_AndruavResala_Ctrl_Camera);

        const Json::Value& jsonConfig = cConfigFile->GetConfigJSON();
        
        Json::Value jsonID;
        jsonID[INTERMODULE_COMMAND_TYPE] =  CMD_TYPE_INTERMODULE;
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
        ms[JSON_INTERMODULE_TIMESTAMP_INSTANCE]     = Json::Int64(time_stamp);

        jsonID[ANDRUAV_PROTOCOL_MESSAGE_CMD] = ms;
                        
        return jsonID;
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

        if (std::strcmp(jMsg[INTERMODULE_COMMAND_TYPE].asCString(),CMD_TYPE_INTERMODULE)==0)
        {
                const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
                const Json::Value moduleID = cmd ["f"]; // ex: "f":{"gr":"1","sd":"pad_uavos_os"}
                PartyID = std::string(moduleID[ANDRUAV_PROTOCOL_SENDER].asCString());
                GroupID = std::string(moduleID[ANDRUAV_PROTOCOL_GROUP_ID].asCString());

                if (!bFirstReceived)
                { 
                        // tell server you dont need to send ID again (z=false).
                        std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Communicator Server Found " <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
                        cUDPClient->SetJSONID (createJSONID(false));
                        bFirstReceived = true;
                }
                return ;
        }

        int messageType = jMsg[ANDRUAV_PROTOCOL_MESSAGE_TYPE].asInt();
        std::cout << "messageType: " << messageType << std::endl;
        switch (messageType)
        {
                case TYPE_AndruavResala_Ctrl_Camera:
                {
                        const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
                        cWEBRTC_Plugin->startImageCapturing(jMsg);
                        
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
                                        ((CUDPClient *)cUDPClient)->SetJSONID (createJSONID(false));
                                break;
                                
                                case RemoteCommand_STREAMVIDEO:
                                        // const bool act = cmd["Act"].asBool();
                                        // if (!act)
                                        // {
                                        //         // act true is not handled here.
                                        //         //TODO: replace act false with hangout signalling or replicate it with another signalling message "hangout"
                                        //         cWEBRTC_Plugin->
                                        // }
                                        ((CUDPClient *)cUDPClient)->SetJSONID (createJSONID(false));
                                break;

                                case RemoteCommand_SWITCHCAM:
                                        //_camera.nextCamera (jmsg.sd);
                                        std::cout << "Key " << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: RemoteCommand_SWITCHCAM" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                                        ((CUDPClient *)cUDPClient)->SetJSONID (createJSONID(false));
                                break;

                        }
                }
                break;

                case TYPE_AndruavMessage_Signaling:
                {
                        const Json::Value cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
                        const Json::Value w = cmd["w"];
    
                        Json::Value const packet = w["packet"];
                        
                        std::cout << _LOG_CONSOLE_TEXT_BOLD_ << "DEBUG: TYPE_AndruavMessage_Signaling" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                        if (!jMsg.isMember(ANDRUAV_PROTOCOL_SENDER))
                        {
                                // cannot send this command as broadcast.
                                return ;                                        
                        }
                        //
                        cWEBRTC_Plugin->ExecuteSignalCommand(jMsg);  
                        if ((packet.isMember("joinme")==true) || (packet.isMember("hangup")==true))
                        {
                                ((CUDPClient *)cUDPClient)->SetJSONID (createJSONID(false));
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