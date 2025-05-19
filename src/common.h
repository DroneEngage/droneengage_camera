#ifndef COMMON_ID
#define COMMON_ID

#define JSON_DIAGNOSTICS 0

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


#include "api/jsep.h"
#include "api/rtc_error.h"
#include "api/candidate.h"

#include "api/audio/audio_device.h"
#include "api/audio/audio_device_defines.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/audio/builtin_audio_processing_builder.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/candidate.h"
#include "api/create_peerconnection_factory.h"
#include "api/enable_media.h"
#include "api/environment/environment_factory.h"
#include "api/media_stream_interface.h"
#include "api/make_ref_counted.h"
#include "api/peer_connection_interface.h"
#include "api/rtc_event_log/rtc_event_log_factory.h"
#include "api/scoped_refptr.h"
#include "api/sequence_checker.h"
#include "api/units/time_delta.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video/i420_buffer.h"
#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "api/video/video_source_interface.h"
#include "api/video_codecs/video_decoder_factory_template.h"
#include "api/video_codecs/video_decoder_factory_template.h"
#include "api/video_codecs/video_decoder_factory_template_dav1d_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_libvpx_vp9_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_open_h264_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp9_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_open_h264_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libaom_av1_adapter.h"
#include "api/video_codecs/video_encoder_factory_template.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "modules/video_capture/video_capture_factory.h"
#include "modules/audio_device/include/audio_device.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "media/base/media_channel.h"
#include "media/base/video_common.h"
#include "media/base/video_adapter.h"
#include "media/base/video_broadcaster.h"
#include "media/engine/simulcast_encoder_adapter.h"
#include "media/engine/internal_encoder_factory.h"
#include "media/engine/simulcast_encoder_adapter.h"

#include "pc/webrtc_sdp.h"
#include "pc/video_track_source.h"
#include "pc/peer_connection_factory.h"
#include "p2p/base/port_interface.h"
#include "p2p/client/basic_port_allocator.h"

#include "api/location.h" // Required for RTC_FROM_HERE
#include "rtc_base/thread.h"
#include "rtc_base/fake_network.h"
#include "rtc_base/socket_address.h"
#include "rtc_base/system/rtc_export.h"
#include "rtc_base/net_helper.h"
#include "rtc_base/synchronization/mutex.h"

// #include "rtc_base/checks.h"
// #include "rtc_base/thread.h"
// #include "rtc_base/thread_annotations.h"
// // //#include "rtc_base/network.h"
// // //#include "rtc_base/network_constants.h"

// #include "rtc_base/synchronization/mutex.h"
// #include "rtc_base/time_utils.h"
// #include "rtc_base/net_helper.h"
// // #include "p2p/base/fake_port_allocator.h"
// // #include "p2p/client/basic_port_allocator.h"


#include "global.h"



#include "./helpers/helpers.hpp"



#include "./helpers/json_nlohmann.hpp"
using Json_de = nlohmann::json;

#include "util/helper.hpp"
#include "util/timer.hpp"
#include "./de_common/messages.hpp"
#include "./de_common/configFile.hpp"

#include "webrtc_callback.hpp"
#include "webrtc_networkManager.hpp"
#include "webrtc/webrtc_video_dev_capturer.hpp"
#include "webrtc/webrtc_capturerTrackSource.hpp"
#include "webrtc/webrtc_userMedia.hpp"
#include "webrtc/webrtc_source.hpp"
#include "webrtc/webrtc_video_encoder_factory.hpp"
#include "webrtc/webrtc_SetSessionDescriptionObserver.hpp"
#include "webrtc/webrtc_peerConnectionManager.hpp"
#include "webrtc/webrtc_fakeAudioCaptureModule.hpp"
#include "webrtc_plugin.hpp"






#endif
