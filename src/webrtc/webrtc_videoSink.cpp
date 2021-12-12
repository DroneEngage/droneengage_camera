
#include "../common.h"


using namespace uavos;


void uavos::stream_webrtc::CVideoSink::OnFrame (const webrtc::VideoFrame& frame)
{
    static bool first = false;
    if (!first)
    {
        std::cout << "On frame from HERE" << std::endl;
        first = true;
    }
}

void uavos::stream_webrtc::CVideoSink::OnDiscardedFrame()
{
    std::cout << "OnDiscardedFrame " << std::endl;
}

