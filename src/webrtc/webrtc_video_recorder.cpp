
#include "../common.h"
#include <chrono>
#include "webrtc_video_recorder.hpp"



using namespace uavos;
using namespace uavos::stream_webrtc;

const int BYTES_PER_PIXEL = 3; /// red, green, & blue
const int FILE_HEADER_SIZE = 14;
const int INFO_HEADER_SIZE = 40;

bool uavos::stream_webrtc::CVideoRecording::startRecording()
{
    m_timer_video.reset();
    stopRecording();
    
    webrtc::MutexLock lock(&m_lock_video);
    std::time_t time_stamp;
    time_stamp = std::time(nullptr);
    m_video_file_name = "v_" + uavos::util::CHelper::getFileTimeStamp() + ".yuv";
    m_video_handler = fopen(m_video_file_name.c_str(), "wb");
    
    m_is_video_recording = true;

    return true;
}
    

bool uavos::stream_webrtc::CVideoRecording::stopRecording()
{
    webrtc::MutexLock lock(&m_lock_video);
    m_is_video_recording = false;
    if (m_video_handler != nullptr)
    {
        fclose(m_video_handler);
        m_video_handler= nullptr;
    }
    
    return true;
}



bool uavos::stream_webrtc::CVideoRecording::screenShot(const uint &image_count, const uint &image_duration)
{
    webrtc::MutexLock lock(&m_lock_image);
    m_image_count     = image_count;
    m_image_duration  = image_duration * 1000; // convert to ms
    m_timer_image.reset();
    return true;
}


int uavos::stream_webrtc::CVideoRecording::printPlane(const uint8_t* buf,
               int width,
               int height,
               int stride) {
    for (int i = 0; i < height; i++, buf += stride) {
        if (fwrite(buf, 1, width, m_video_handler) != static_cast<unsigned int>(width))
        return -1;
    }
    return 0;
}


int uavos::stream_webrtc::CVideoRecording::printVideoFrame(const webrtc::VideoFrame& frame) 
{
    
    if (m_timer_video.elapsed_milli() < 100) return 0;

    m_timer_video.reset();
    
    webrtc::MutexLock lock(&m_lock_video);
    if (!m_is_video_recording) return 0;

    if (m_video_handler == nullptr) return -1;
    
    webrtc::I420BufferInterface &frame_I420 = *frame.video_frame_buffer()->ToI420();
    int width = frame_I420.width();
    int height = frame_I420.height();
    int chroma_width = frame_I420.ChromaWidth();
    int chroma_height = frame_I420.ChromaHeight();

    if (printPlane(frame_I420.DataY(), width, height, frame_I420.StrideY()) < 0) {
        return -1;
    }
    if (printPlane(frame_I420.DataU(), chroma_width, chroma_height, frame_I420.StrideU()) < 0) {
        return -1;
    }
    if (printPlane(frame_I420.DataV(), chroma_width, chroma_height, frame_I420.StrideV()) < 0) {
        return -1;
    }

    return 0;
}



unsigned char* uavos::stream_webrtc::CVideoRecording::createBitmapFileHeader (const uint& height, const uint& stride)
{
    int fileSize = FILE_HEADER_SIZE + INFO_HEADER_SIZE + (stride * height);

    static unsigned char fileHeader[] = {
        0,0,     /// signature
        0,0,0,0, /// image file size in bytes
        0,0,0,0, /// reserved
        0,0,0,0, /// start of pixel array
    };

    fileHeader[ 0] = (unsigned char)('B');
    fileHeader[ 1] = (unsigned char)('M');
    fileHeader[ 2] = (unsigned char)(fileSize      );
    fileHeader[ 3] = (unsigned char)(fileSize >>  8);
    fileHeader[ 4] = (unsigned char)(fileSize >> 16);
    fileHeader[ 5] = (unsigned char)(fileSize >> 24);
    fileHeader[10] = (unsigned char)(FILE_HEADER_SIZE + INFO_HEADER_SIZE);

    return fileHeader;
}

unsigned char* uavos::stream_webrtc::CVideoRecording::createBitmapInfoHeader (const uint&  height, const uint&  width)
{
    static unsigned char infoHeader[] = {
        0,0,0,0, /// header size
        0,0,0,0, /// image width
        0,0,0,0, /// image height
        0,0,     /// number of color planes
        0,0,     /// bits per pixel
        0,0,0,0, /// compression
        0,0,0,0, /// image size
        0,0,0,0, /// horizontal resolution
        0,0,0,0, /// vertical resolution
        0,0,0,0, /// colors in color table
        0,0,0,0, /// important color count
    };
    //https://stackoverflow.com/questions/26144955/after-writing-bmp-file-image-is-flipped-upside-down
    // inverse height as data for somereason is comming reversed.
    int h = -1 * height;
    infoHeader[ 0] = (unsigned char)(INFO_HEADER_SIZE);
    infoHeader[ 4] = (unsigned char)(width      );
    infoHeader[ 5] = (unsigned char)(width >>  8);
    infoHeader[ 6] = (unsigned char)(width >> 16);
    infoHeader[ 7] = (unsigned char)(width >> 24);
    infoHeader[ 8] = (unsigned char)(h);
    infoHeader[ 9] = (unsigned char)(h >>  8);
    infoHeader[10] = (unsigned char)(h >> 16);
    infoHeader[11] = (unsigned char)(h >> 24);
    infoHeader[12] = (unsigned char)(1);
    infoHeader[14] = (unsigned char)(BYTES_PER_PIXEL*8);

    return infoHeader;
}


int  uavos::stream_webrtc::CVideoRecording::saveFrameAsRGB( webrtc::VideoFrame& frame)
{
    
    webrtc::MutexLock lock(&m_lock_image);

    if (m_image_count<=0)
    {
        return 0;
    }
  
    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: m_image_count: " << std::to_string(m_image_count) << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif  
        
    if ((m_image_duration!=0) && (m_timer_image.elapsed_milli() < m_image_duration))
    {
        // too soon. wait more time.
        return 0;
    }
    m_timer_image.reset();
    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: m_image_duration: " << std::to_string(m_image_duration) <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif  
    
    m_image_count--;

    // Check https://rawpixels.net/
    size_t file_size = frame.width() * frame.height() * BYTES_PER_PIXEL;
    std::unique_ptr<uint8_t[]> res_rgb_buffer(new uint8_t[file_size]);
    
    // convert to RGB
    webrtc::ConvertFromI420(frame, webrtc::VideoType::kRGB24, 0,
                                    res_rgb_buffer.get());

    // choose file name
    std::string output_file_name = "img_" + uavos::util::CHelper::getFileTimeStamp() + "_" + std::to_string(m_image_count) + ".bmp";
    
    // open file
    FILE* image_handler = fopen(output_file_name.c_str(), "wb");

    // create BMP Header
    int widthInBytes = frame.width() * BYTES_PER_PIXEL;
    unsigned char padding[3] = {0, 0, 0};
    int paddingSize = (4 - (widthInBytes) % 4) % 4;
    int stride = (widthInBytes) + paddingSize;
    unsigned char* fileHeader = createBitmapFileHeader(frame.height(), stride);
    unsigned char* infoHeader = createBitmapInfoHeader(frame.height(), frame.width());
    
    // write file header
    fwrite(fileHeader, 1, FILE_HEADER_SIZE, image_handler);
    fwrite(infoHeader, 1, INFO_HEADER_SIZE, image_handler);
  
    // write image data
    fwrite(res_rgb_buffer.get(), 1, file_size, image_handler);

    fclose(image_handler);
    return 0;
}