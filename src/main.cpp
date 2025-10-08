// #include <signal.h>
#include <stdio.h>
#include <getopt.h>
#include "common.h"
#include "webrtc_plugin.hpp"
#include "./de_common/de_databus/de_module.hpp"
#include "./de_common/helpers/util_rpi.hpp"
#include "./de_common/de_databus/configFile.hpp"
#include "./de_common/de_databus/localConfigFile.hpp"
#include "./webrtc/webrtc_facade.hpp"

using Json_de = nlohmann::json;

#include "./de_common/helpers/getopt_cpp.hpp"

#include "version.h"

using namespace de;

#define MESSAGE_FILTER {TYPE_AndruavMessage_RemoteExecute,         \
                        TYPE_AndruavMessage_Signaling,             \
                        TYPE_AndruavMessage_Ctrl_Cameras,          \
                        TYPE_AndruavModule_Location_Info,          \
                        TYPE_AndruavMessage_CONFIG_ACTION,         \
                        TYPE_AndruavMessage_DUMMY}

static webrtc::Thread *the_default_thread = webrtc::Thread::Current();
std::time_t instance_time_stamp;

// DroneEngage Current PartyID read from communicator
std::string PartyID;
// DroneEngage Current GroupID read from communicator
std::string GroupID;
std::string ModuleKey;

de::CConfigFile &cConfigFile = CConfigFile::getInstance();
de::CLocalConfigFile &cLocalConfigFile = de::CLocalConfigFile::getInstance();

de::comm::CModule &cModule = de::comm::CModule::getInstance();

CWEBRTC_Plugin &cWEBRTC_Plugin = CWEBRTC_Plugin::getInstance();

/**
 * @brief hardware serial number
 *
 */
static std::string hardware_serial;
static std::mutex exit_mutex;
static std::condition_variable exit_cv;
static std::atomic<bool> m_exit{false};

// void quit_handler( int sig );

void initSerial()
{
    // helpers::CUtil_Rpi::getInstance().get_cpu_serial(hardware_serial);
    // hardware_serial.append(get_linux_machine_id());
    hardware_serial.append("ss");
}

void onReceive(const char *message, int len, Json_de jMsg);
pthread_t createUDPSocket_thread;

static std::string configName = "de_camera.config.module.json";
static std::string localConfigName = "de_camera.config.local";

/**
 * @brief display version info
 *
 */
void _version(void)
{
    std::cout << std::endl
              << _SUCCESS_CONSOLE_BOLD_TEXT_ "Drone-Engage Camera Plugin version " << _INFO_CONSOLE_TEXT << version_string << _NORMAL_CONSOLE_TEXT_ << std::endl;
}

/**
 * @brief display help for -h command argument.
 *
 */
void _usage(void)
{
    _version();
    std::cout << std::endl
              << _INFO_CONSOLE_TEXT "Options" << _NORMAL_CONSOLE_TEXT_ << std::ends;
    std::cout << std::endl
              << _INFO_CONSOLE_TEXT "\t--config:      -c ./config.json   default [./config.module.json]" << _NORMAL_CONSOLE_TEXT_ << std::ends;
    std::cout << std::endl
              << _INFO_CONSOLE_TEXT "\t--version:     -v" << _NORMAL_CONSOLE_TEXT_ << std::endl;
}

void createJSONID(const bool resend)
{
    const Json_de cameraList = cWEBRTC_Plugin.getDeviceListAsJSON();
    cModule.appendExtraField("m", cameraList);
    cModule.createJSONID(resend);
}

void initArguments(int argc, char *argv[])
{
    int opt;
    const struct GetOptLong::option options[] = {
        {"config", true, 0, 'c'},
        {"version", false, 0, 'v'},
        {"help", false, 0, 'h'},
        {0, false, 0, 0}};
    // adding ':' means there is extra parameter needed
    GetOptLong gopt(argc, argv, "c:vh",
                    options);

    /*
      parse command line options
     */
    while ((opt = gopt.getoption()) != -1)
    {
        switch (opt)
        {
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

    const Json_de &jsonConfig = cConfigFile.GetConfigJSON();
    de::CLocalConfigFile &cLocalConfigFile = de::CLocalConfigFile::getInstance();

    // UDP Server
    cModule.defineModule(
        MODULE_CLASS_VIDEO,
        jsonConfig["module_id"],
        cLocalConfigFile.getStringField("module_key"),
        version_string,
        Json_de::array(MESSAGE_FILTER));

    cModule.addModuleFeatures(MODULE_FEATURE_CAPTURE_IMAGE);
    cModule.addModuleFeatures(MODULE_FEATURE_CAPTURE_VIDEO);
    cModule.setHardware(hardware_serial, ENUM_HARDWARE_TYPE::HARDWARE_TYPE_CPU);

    cModule.setMessageOnReceive(&onReceive);

    createJSONID(false);

    int udp_chunk_size = DEFAULT_UDP_DATABUS_PACKET_SIZE;

    if (validateField(jsonConfig, "s2s_udp_packet_size", Json_de::value_t::string))
    {
        udp_chunk_size = std::stoi(jsonConfig["s2s_udp_packet_size"].get<std::string>());
    }
    else
    {
        std::cout << _LOG_CONSOLE_BOLD_TEXT << "WARNING:" << _INFO_CONSOLE_TEXT << " MISSING FIELD " << _ERROR_CONSOLE_BOLD_TEXT_ << "s2s_udp_packet_size " << _INFO_CONSOLE_TEXT << "is missing in config file. default value " << _ERROR_CONSOLE_BOLD_TEXT_ << std::to_string(DEFAULT_UDP_DATABUS_PACKET_SIZE) << _INFO_CONSOLE_TEXT << " is used." << _NORMAL_CONSOLE_TEXT_ << std::endl;
    }

    cModule.init(jsonConfig["s2s_udp_target_ip"].get<std::string>(),
                 std::stoi(jsonConfig["s2s_udp_target_port"].get<std::string>()),
                 jsonConfig["s2s_udp_listening_ip"].get<std::string>(),
                 std::stoi(jsonConfig["s2s_udp_listening_port"].get<std::string>()),
                 udp_chunk_size);
}

/**
 * initialize components
 **/
void init(int argc, char *argv[])
{
    instance_time_stamp = std::time(nullptr);

    // signal(SIGINT,quit_handler);
    // signal(SIGTERM,quit_handler);

    // initialize serial
    initSerial();

    initArguments(argc, argv);

    // Reading Configuration
    std::cout << std::endl
              << "=================== " << "STARTING PLUGIN ===================" << std::endl;
    _version();

    cConfigFile.initConfigFile(configName.c_str());
    cLocalConfigFile.InitConfigFile(localConfigName.c_str());

    ModuleKey = cLocalConfigFile.getStringField("module_key");
    if (ModuleKey == "")
    {
        ModuleKey = std::to_string(get_time_usec());
        cLocalConfigFile.addStringField("module_key", ModuleKey.c_str());
        cLocalConfigFile.apply();
    }

    Json_de jsonConfig = cConfigFile.GetConfigJSON();

    const std::string ModuleID = jsonConfig["module_id"].get<std::string>();

    // https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
    std::cout << _LOG_CONSOLE_BOLD_TEXT << "DroneEngage Plugin Module: " << _SUCCESS_CONSOLE_BOLD_TEXT_ << ModuleID << _NORMAL_CONSOLE_TEXT_ << std::endl;
    std::cout << _LOG_CONSOLE_BOLD_TEXT << "Class Type: " << _SUCCESS_CONSOLE_BOLD_TEXT_ << "camera" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    std::cout << std::asctime(std::localtime(&instance_time_stamp)) << instance_time_stamp << " seconds since the Epoch" << std::endl;

    // INIT WEBRTC

    cWEBRTC_Plugin.initCameras();
    if (!jsonConfig.contains("camera"))
    {
        // if no camera record or old style data then assume from (0 to 999)
        if (!jsonConfig.contains("camera"))
        {
            // TODO: REMOVE THIS IN LATER VERSIONS.
            //  backward compatibility - will be removed soon.
            const int minCameraIndex = jsonConfig.contains("camera_start_index") ? jsonConfig["camera_start_index"].get<int>() : 0;
            const int maxCameraIndex = jsonConfig.contains("camera_end_index") ? jsonConfig["camera_end_index"].get<int>() : 999;

            // Keep this from 0 to 999
            cWEBRTC_Plugin.addCameraByRange(minCameraIndex, maxCameraIndex);
        }
    }
    else
    {
        // the logic:
        // camera entry should exists.
        // it should contains wither camera_list
        // or camera_start_index / camera_end_index
        
        Json_de camera = jsonConfig["camera"];
        
        if (camera.contains("camera_list"))
        {
            Json_de jsonCameraList = camera["camera_list"];

            size_t numItems = jsonCameraList.size();

            if (numItems == 0)
            {
                std::cout << _ERROR_CONSOLE_BOLD_TEXT_ << "ERROR in Config File" << _INFO_BOLD_CONSOLE_TEXT << " camera_list" << _ERROR_CONSOLE_BOLD_TEXT_ << " field contains no entries." << _NORMAL_CONSOLE_TEXT_ << std::endl;
                std::cout << _NORMAL_CONSOLE_TEXT_ << "Suggest:" << _TEXT_BOLD_HIGHTLITED_ << "Please define at least one camera." << _NORMAL_CONSOLE_TEXT_ << std::endl;
            }

            for (auto cameraItem : jsonCameraList)
            {
                if (cameraItem["name"].get<std::string>().empty())
                    continue; // most propably it is an extra comma after last field.

                if (cameraItem.contains("device_num"))
                {
                    std::cout << _LOG_CONSOLE_BOLD_TEXT << "Trying to init: " << _INFO_CONSOLE_TEXT << cameraItem["name"].get<std::string>() << _LOG_CONSOLE_BOLD_TEXT << " \\dev\\video " << _INFO_CONSOLE_TEXT << cameraItem["device_num"].get<int>() << _NORMAL_CONSOLE_TEXT_ << std::endl;

                    if (!cWEBRTC_Plugin.addCameraByID(cameraItem["name"].get<std::string>(), cameraItem["device_num"].get<int>()))
                    {
                        std::cout << _ERROR_CONSOLE_TEXT_ << "failed" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                    }

                    continue;
                }

                if (cameraItem.contains("device_name"))
                {
                    std::cout << _LOG_CONSOLE_BOLD_TEXT << "Trying to init: " << _INFO_CONSOLE_TEXT << cameraItem["name"].get<std::string>() << _LOG_CONSOLE_BOLD_TEXT << "  " << _INFO_CONSOLE_TEXT << cameraItem["device_name"].get<std::string>() << _NORMAL_CONSOLE_TEXT_ << std::endl;

                    if (!cWEBRTC_Plugin.addCameraByDeviceName(cameraItem["name"].get<std::string>(), cameraItem["device_name"].get<std::string>()))
                    {
                        std::cout << _ERROR_CONSOLE_TEXT_ << "failed" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                    }
                    continue;
                }
            }
        }
        else
        {
            const int minCameraIndex = camera.contains("camera_start_index") ? camera["camera_start_index"].get<int>() : 0;
            const int maxCameraIndex = camera.contains("camera_end_index") ? camera["camera_end_index"].get<int>() : 999;

            std::cout << _SUCCESS_CONSOLE_BOLD_TEXT_ << "Camera Scan from /dev/video" <<   _INFO_CONSOLE_TEXT << minCameraIndex << _SUCCESS_CONSOLE_BOLD_TEXT_ << " to /dev/video"  << _INFO_CONSOLE_TEXT << maxCameraIndex << _NORMAL_CONSOLE_TEXT_ <<  std::endl;
                    
            cWEBRTC_Plugin.addCameraByRange(minCameraIndex, maxCameraIndex);
        }
    }

    // BUG: for unknown reason calling "InitializePeerConnection" with the "cWEBRTC_Plugin.init" generates strange error.type
    //  error is: if you send JSON of camera lists and the list length is one then it will corrupt  and will not talk to server.
    cWEBRTC_Plugin.InitializePeerConnection();

    initUavosModule(argc, argv);
}

void uninit()
{
    m_exit.store(true);
}

/*
    Process received messages from UDP client.
    most propably received from UAVOS_Communicator
*/
void onReceive(const char *message, int len, Json_de jMsg)
{

    if (!jMsg.contains(ANDRUAV_PROTOCOL_MESSAGE_CMD))
        return;

    const int messageType = jMsg[ANDRUAV_PROTOCOL_MESSAGE_TYPE].get<int>();
    const Json_de cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];

    if (std::strcmp(jMsg[INTERMODULE_ROUTING_TYPE].get<std::string>().c_str(), CMD_TYPE_INTERMODULE) == 0)
    {

        if (messageType == TYPE_AndruavModule_ID)
        {
            PartyID = cModule.getPartyId();
            GroupID = cModule.getGroupId();

            return;
        }
    }

#ifdef DDEBUG
    std::cout << "messageType: " << messageType << std::endl;
#endif

    switch (messageType)
    {

    case TYPE_AndruavMessage_CONFIG_ACTION:
    {

        std::string module_key = "";

        if (!validateField(cmd, "a", Json_de::value_t::number_unsigned))
            return;

        if (validateField(cmd, "b", Json_de::value_t::string))
        {
            module_key = de::comm::CModule::getInstance().getModuleKey();

            if (module_key != cmd["b"].get<std::string>())
                return;
        }
        
        int action = cmd["a"].get<int>();
        switch (action)
        {
        case CONFIG_ACTION_Restart:
            /* code */
            exit(0);
            break;

        case CONFIG_ACTION_APPLY_CONFIG:
        {
            Json_de config = cmd["c"];
            std::cout << config << std::endl;
            de::CConfigFile &cConfigFile = CConfigFile::getInstance();
            cConfigFile.updateJSON(config.dump());
        }
        break;

        case CONFIG_REQUEST_FETCH_CONFIG_TEMPLATE:
        {
            if (!jMsg.contains(ANDRUAV_PROTOCOL_SENDER)) return;
            std::string sender = jMsg[ANDRUAV_PROTOCOL_SENDER].get<std::string>();
#ifdef DEBUG
            std::cout << std::endl << _INFO_CONSOLE_TEXT << "CONFIG_REQUEST_FETCH_CONFIG_TEMPLATE" << _NORMAL_CONSOLE_TEXT_ << std::endl;
#endif
            std::ifstream file("template.json");
            if (!file.is_open()) {
                std::cout << std::endl << _ERROR_CONSOLE_BOLD_TEXT_ << "cannot read template.json" << _NORMAL_CONSOLE_TEXT_ << std::endl;
                //CFCBFacade::getInstance().sendErrorMessage(std::string(), 0, ERROR_3DR, NOTIFICATION_TYPE_ERROR, "cannot read template.json");
                Json_de empty_file_content_json = {};
                de::fcb::CFCBFacade::getInstance().API_sendConfigTemplate(sender, module_key, empty_file_content_json, true);
                return;
            }

            std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            Json_de file_content_json = Json_de::parse(file_content);
            de::fcb::CFCBFacade::getInstance().API_sendConfigTemplate(sender, module_key, file_content_json, true);
        }
        break;
        default:
            break;
        }
    }
    break;

    case TYPE_AndruavModule_Location_Info:
    {
        cWEBRTC_Plugin.updateLocationInfo(jMsg);
    }
    break;

    case TYPE_AndruavMessage_Ctrl_Cameras:
    {
        cWEBRTC_Plugin.startImageCapturing(jMsg);
        createJSONID(false);
    }
    break;

    case TYPE_AndruavMessage_RemoteExecute:
    {
        const int remoteCommand = cmd["C"].get<int>();
#ifdef DEBUG
        std::cout << "cmd: " << remoteCommand << std::endl;
#endif
        switch (remoteCommand)
        {
        case RemoteCommand_RECORDVIDEO:
        {
            std::cout << "Key " << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: RemoteCommand_RECORDVIDEO" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            std::cout << _LOG_CONSOLE_BOLD_TEXT << "DEBUG: TYPE_AndruavMessage_Signaling" << _NORMAL_CONSOLE_TEXT_ << std::endl;
            if (!jMsg.contains(ANDRUAV_PROTOCOL_SENDER))
            {
                // cannot send this command as broadcast.
                return;
            }

            bool startIfTrue = cmd["Act"].get<bool>();
            std::string channelName = cmd["T"].get<std::string>();

            if (startIfTrue == true)
            {
                cWEBRTC_Plugin.startVideoRecording(jMsg);
            }
            else
            {
                cWEBRTC_Plugin.stopVideoRecording(jMsg);
            }

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

        case RemoteCommand_ROTATECAM:
        {
            const Json_de cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
            cWEBRTC_Plugin.rotateCameraFrame(jMsg);
        }
        break;
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

        std::cout << _LOG_CONSOLE_BOLD_TEXT << "INFO: TYPE_AndruavMessage_Signaling" << _NORMAL_CONSOLE_TEXT_ << std::endl;
        if (!jMsg.contains(ANDRUAV_PROTOCOL_SENDER))
        {
            // cannot send this command as broadcast.
            return;
        }

        cWEBRTC_Plugin.ExecuteSignalCommand(jMsg);
        if ((packet.contains("joinme") == true) || (packet.contains("hangup") == true))
        {
            const Json_de cameraList = cWEBRTC_Plugin.getDeviceListAsJSON();
            createJSONID(false);
        }
    }
    break;
    }
}

int main(int argc, char *argv[])
{
    init(argc, argv);

    // Wait for exit signal
    {
        std::unique_lock<std::mutex> lock(exit_mutex);
        exit_cv.wait(lock, []
                     { return m_exit.load(); });
    }

    cWEBRTC_Plugin.cleaning();
}