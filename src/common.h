

#include <queue>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <ctime> 

#include <unistd.h> //sleep
#include <sstream>
#include <fstream>
#include <atomic>
#include <map>
#include <string.h>


#define BUILD_NINJA 
#ifdef BUILD_NINJA
#include "rtc_base/strings/json.h"
#else
#include <jsoncpp/json/json.h>
#endif

#include "absl/algorithm/container.h"
#include "absl/base/macros.h"
#include "absl/types/optional.h"

#include "api/jsep.h"
#include "api/rtc_error.h"
#include "rtc_base/network_constants.h"
#include "media/base/video_common.h"
#include "rtc_base/network.h"
#include "rtc_base/async_invoker.h"
#include "rtc_base/network_constants.h"
#include "p2p/client/basic_port_allocator.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "rtc_base/system/rtc_export.h"
#include "media/engine/internal_encoder_factory.h"
//#include "media/base/video_capturer.h"


//#include "media/engine/webrtcvideocapturerfactory.h"
#include "rtc_base/fake_network.h"
#include "pc/video_track_source.h"
#include "api/video/i420_buffer.h"
#include "api/video/video_frame.h"
#include "rtc_base/synchronization/mutex.h"
#include "api/peer_connection_interface.h"
#include "api/video/video_sink_interface.h"
#include "api/video/video_source_interface.h"
#include "modules/video_capture/video_capture_factory.h"
#include "media/base/video_broadcaster.h"
#include "media/base/video_adapter.h"
#include "rtc_base/message_handler.h"
#include "api/create_peerconnection_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "modules/audio_device/include/audio_device.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "rtc_tools/video_file_writer.h"
#include "global.h"

#include "./helpers/json_nlohmann.hpp"
using Json_de = nlohmann::json;

#include "util/helper.hpp"
#include "util/timer.hpp"
#include "./uavos_common/messages.hpp"
#include "configFile.hpp"
#include "webrtc_callback.hpp"
#include "p2p/base/fake_port_allocator.h"
#include "webrtc_networkManager.hpp"
#include "webrtc/webrtc_video_dev_capturer.hpp"
#include "webrtc/webrtc_capturerTrackSource.hpp"
#include "webrtc/webrtc_userMedia.hpp"
#include "webrtc/webrtc_source.hpp"
#include "webrtc/webrtc_video_encoder_factory.hpp"
//#include "webrtc_PeerConnectionObserver.hpp"
#include "webrtc/webrtc_SetSessionDescriptionObserver.hpp"
#include "webrtc/webrtc_peerConnectionManager.hpp"
#include "webrtc_plugin.hpp"



#ifndef COMMON_ID
#define COMMON_ID

#endif
