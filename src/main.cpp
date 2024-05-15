#include <stdio.h>
#include <getopt.h>
#include "common.h"
#include "webrtc_plugin.hpp"
#include "./de_common/de_module.hpp"
#include "./helpers/json_nlohmann.hpp"
#include "./helpers/helpers.hpp"
#include "./helpers/util_rpi.hpp"

using Json_de = nlohmann::json;

#include "./helpers/getopt_cpp.hpp"

#include "version.h"

using namespace de;

#define MESSAGE_FILTER {TYPE_AndruavMessage_RemoteExecute,\
                        TYPE_AndruavMessage_Signaling,\
                        TYPE_AndruavMessage_Ctrl_Cameras, \
                        TYPE_AndruavModule_Location_Info, \
                        TYPE_AndruavMessage_DUMMY}


std::time_t instance_time_stamp;

// DroneEngage Current PartyID read from communicator
std::string  PartyID;
// DroneEngage Current GroupID read from communicator
std::string  GroupID;
std::string  ModuleID;
std::string  ModuleKey;

CConfigFile& cConfigFile = CConfigFile::getInstance();

de::comm::CModule& cModule= de::comm::CModule::getInstance();

CWEBRTC_Plugin * cWEBRTC_Plugin;

/**
 * @brief hardware serial number
 * 
 */
static std::string hardware_serial;

void initSerial()
{
    //helpers::CUtil_Rpi::getInstance().get_cpu_serial(hardware_serial);
    //hardware_serial.append(get_linux_machine_id());
    hardware_serial.append("ss");
}


void onReceive (const char * message, int len, Json_de jMsg);
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

void createJSONID(const bool resend)
{
    const Json_de cameraList = cWEBRTC_Plugin->getDeviceListAsJSON();
    cModule.appendExtraField ("m", cameraList);
    cModule.createJSONID(resend);
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


void initUavosModule(int argc, char *argv[])
{

    const Json_de& jsonConfig = cConfigFile.GetConfigJSON();

    // UDP Server
    cModule.defineModule(
        MODULE_CLASS_VIDEO,
        jsonConfig["module_id"],
        jsonConfig["module_key"],
        version_string,
        Json_de::array(MESSAGE_FILTER)
    );
    
    cModule.addModuleFeatures(MODULE_FEATURE_CAPTURE_IMAGE);
    cModule.addModuleFeatures(MODULE_FEATURE_CAPTURE_VIDEO);
    cModule.setHardware(hardware_serial, ENUM_HARDWARE_TYPE::HARDWARE_TYPE_CPU);
    
    cModule.setMessageOnReceive (&onReceive);
    
    createJSONID (false);

    int udp_chunk_size = DEFAULT_UDP_DATABUS_PACKET_SIZE;
    
    if (validateField(jsonConfig, "s2s_udp_packet_size",Json_de::value_t::number_unsigned)) 
    {
        udp_chunk_size = jsonConfig["s2s_udp_packet_size"].get<int>();
    }
    else
    {
        std::cout << _LOG_CONSOLE_BOLD_TEXT << "WARNING:" << _INFO_CONSOLE_TEXT << " MISSING FIELD " << _ERROR_CONSOLE_BOLD_TEXT_ << "s2s_udp_packet_size " <<  _INFO_CONSOLE_TEXT << "is missing in config file. default value " << _ERROR_CONSOLE_BOLD_TEXT_  << "8160 " <<  _INFO_CONSOLE_TEXT <<  "is used." << _NORMAL_CONSOLE_TEXT_ << std::endl;    
    }

    cModule.init(jsonConfig["s2s_udp_target_ip"].get<std::string>(),
            std::stoi(jsonConfig["s2s_udp_target_port"].get<std::string>()),
            jsonConfig["s2s_udp_listening_ip"].get<std::string>() ,
            std::stoi(jsonConfig["s2s_udp_listening_port"].get<std::string>()),
            udp_chunk_size);

}


/**
 * initialize components
 **/
void init (int argc, char *argv[]) 
{
    instance_time_stamp = std::time(nullptr);
    
    //initialize serial
    initSerial();

    initArguments (argc, argv);

    // Reading Configuration
    std::cout << std::endl << "=================== " << "STARTING PLUGIN ===================" << std::endl;
    _version();


    cConfigFile.initConfigFile (configName.c_str());
    Json_de jsonConfig = cConfigFile.GetConfigJSON();
    
    ModuleID = jsonConfig["module_id"].get<std::string>();
    ModuleKey = jsonConfig["module_key"].get<std::string>();
    std::string  ModuleKey;

    //https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
    std::cout << _LOG_CONSOLE_BOLD_TEXT << "DroneEngage Plugin Module: " << _SUCCESS_CONSOLE_BOLD_TEXT_ <<  ModuleID << _NORMAL_CONSOLE_TEXT_ << std::endl;
    std::cout << _LOG_CONSOLE_BOLD_TEXT << "Class Type: " << _SUCCESS_CONSOLE_BOLD_TEXT_<< "camera" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    std::cout << std::asctime(std::localtime(&instance_time_stamp)) << instance_time_stamp << " seconds since the Epoch" << std::endl;
    
    // INIT WEBRTC
    cWEBRTC_Plugin = &CWEBRTC_Plugin::getInstance(); 
    
    const bool singleCameraMode = jsonConfig.contains("one_session_per_camera")?jsonConfig["one_session_per_camera"].get<bool>():true;
    cWEBRTC_Plugin->initCameras(singleCameraMode);
    if (jsonConfig.contains("camera_list"))
    {
        Json_de jsonCameraList= jsonConfig["camera_list"];
        
        size_t numItems = jsonCameraList.size();

        if (numItems==0)
        {
            std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "ERROR in Config File" << _INFO_BOLD_CONSOLE_TEXT << " camera_list" << _ERROR_CONSOLE_BOLD_TEXT_ << " field contains no entries." << _NORMAL_CONSOLE_TEXT_ << std::endl;
            std::cout << _NORMAL_CONSOLE_TEXT_ << "Suggest:" << _TEXT_BOLD_HIGHTLITED_ << "Please define at least one camera." << _NORMAL_CONSOLE_TEXT_ << std::endl;
        }

        for (auto cameraItem : jsonCameraList)
        {
            if (cameraItem["name"].get<std::string>().empty()) continue; // most propably it is an extra comma after last field.
            std::cout << _LOG_CONSOLE_BOLD_TEXT << "Trying to init: " << _INFO_CONSOLE_TEXT << cameraItem["name"].get<std::string>() << _LOG_CONSOLE_BOLD_TEXT << " \\dev\\video" << _INFO_CONSOLE_TEXT << cameraItem["device_num"].get<int>() << _NORMAL_CONSOLE_TEXT_ << std::endl;
            if (!cWEBRTC_Plugin->addCameraByID(cameraItem["name"].get<std::string>(), cameraItem["device_num"].get<int>()))
            {
                std::cout << _ERROR_CONSOLE_TEXT_ << "failed" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            }
        }
	}
    else
    {
        const int minCameraIndex = jsonConfig.contains("camera_start_index")?jsonConfig["camera_start_index"].get<int>():0;
        const int maxCameraIndex = jsonConfig.contains("camera_end_index")?jsonConfig["camera_end_index"].get<int>():999;
        cWEBRTC_Plugin->addCameraByRange(minCameraIndex, maxCameraIndex);
    }

    //BUG: for unknown reason calling "InitializePeerConnection" with the "cWEBRTC_Plugin->init" generates strange error.type
    // error is: if you send JSON of camera lists and the list length is one then it will corrupt  and will not talk to server.
    cWEBRTC_Plugin->InitializePeerConnection();
    
    initUavosModule (argc,argv);
}


void uninit ()
{
   delete cWEBRTC_Plugin;
}


/*
    Process received messages from UDP client.
    most propably received from UAVOS_Communicator
*/
void onReceive (const char * message, int len, Json_de jMsg)
{
        
    // try
    // {
        /* code */
        if (!jMsg.contains(ANDRUAV_PROTOCOL_MESSAGE_CMD)) return ;
        
        const int messageType = jMsg[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
        const Json_de cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            
        if (std::strcmp(jMsg[INTERMODULE_ROUTING_TYPE].get<std::string>().c_str(),CMD_TYPE_INTERMODULE)==0)
        {
            
        
            if (messageType== TYPE_AndruavModule_ID)
            {
                PartyID = cModule.getPartyId();
                GroupID = cModule.getGroupId();
                
                return ;
            }

            
        }

        std::cout << "messageType: " << messageType << std::endl;
        switch (messageType)
        {
            case TYPE_AndruavModule_Location_Info:
            {
                cWEBRTC_Plugin->updateLocationInfo(jMsg);
            }
            break;

            case TYPE_AndruavMessage_Ctrl_Cameras:
            {
                cWEBRTC_Plugin->startImageCapturing(jMsg);
                createJSONID (false);
            }
            break;

            case TYPE_AndruavMessage_RemoteExecute:
            {
                const int remoteCommand = cmd["C"].get<int>();
                std::cout << "cmd: " << remoteCommand << std::endl;
                switch (remoteCommand)
                {
                    case RemoteCommand_RECORDVIDEO:
                    {
                        std::cout << "Key " << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: RemoteCommand_RECORDVIDEO" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                        std::cout << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: TYPE_AndruavMessage_Signaling" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                        if (!jMsg.contains(ANDRUAV_PROTOCOL_SENDER))
                        {
                            // cannot send this command as broadcast.
                            return ;                    
                        }
                        cWEBRTC_Plugin->processVideoRecording(jMsg);   
                        createJSONID(false);
                    }
                    break;
                    
                    
                    case RemoteCommand_STREAMVIDEO:
                    {
                        // const bool act = cmd["Act"].asBool();
                        // if (!act)
                        // {
                        //     // act true is not handled here.
                        //     //TODO: replace act false with hangout signalling or replicate it with another signalling message "hangout"
                        //     cWEBRTC_Plugin->
                        // }
                        createJSONID(false);
                    }
                    break;

                    case RemoteCommand_SWITCHCAM:
                    {
                        //_camera.nextCamera (jmsg.sd);
                        std::cout << "Key " << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: RemoteCommand_SWITCHCAM" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                        createJSONID(false);
                    }
                    break;
                }
            }
            break;

            case TYPE_AndruavMessage_Signaling:
            {
                
                if (!cmd.contains("w")) return ;
                
                const Json_de w = cmd["w"];
        
                if (!w.contains("packet")) return ;
                
                Json_de const packet = w["packet"];
                
                std::cout << _LOG_CONSOLE_BOLD_TEXT << "INFO: TYPE_AndruavMessage_Signaling" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                if (!jMsg.contains(ANDRUAV_PROTOCOL_SENDER))
                {
                    // cannot send this command as broadcast.
                    return ;                    
                }
                //
                cWEBRTC_Plugin->ExecuteSignalCommand(jMsg);  
                if ((packet.contains("joinme")==true) || (packet.contains("hangup")==true))
                {
                    const Json_de cameraList = cWEBRTC_Plugin->getDeviceListAsJSON();
                    createJSONID(false);
                }
            }
            break;
        } 
        
    // }
    // catch(const std::exception& e)
    // {
    //     std::cerr << e.what() << '\n';
    // }
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