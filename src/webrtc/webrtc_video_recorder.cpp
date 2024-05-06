
#include "../common.h"
#include <chrono>
#include "webrtc_video_recorder.hpp"
#include <thread>         // std::thread

#include "../webrtc_plugin.hpp"
#include "../helpers/json_nlohmann.hpp"
using Json_de = nlohmann::json;

extern "C" {
#if defined(USE_SYSTEM_LIBJPEG)
#include <jpeglib.h>
#else
// Include directory supplied by gn
#include "jpeglib.h"  // NOLINT
#endif
}

#include "../3rdparty/LodePNG/lodepng.h"

using namespace uavos;
using namespace uavos::stream_webrtc;

const int BYTES_PER_PIXEL_RGB24 = 3; /// red, green, & blue
const int BYTES_PER_PIXEL_RGBA = 4; /// red, green, & blue
const int FILE_HEADER_SIZE = 14;
const int INFO_HEADER_SIZE = 40;

const std::string uavos::stream_webrtc::CVideoRecording::getMediaFolderPath() const
{
    Json_de jsonConfig = CConfigFile::getInstance().GetConfigJSON();
    if (jsonConfig.contains("media_folder") == false) 
    {
        return std::string();
    }
    
    const std::string file_path = jsonConfig["media_folder"].get<std::string>() + "/";
   
    return file_path;
}

/**
 * @brief reads "media_image_png" from config file.
 * default: true.
 * 
 * @return true 
 * @return false 
 */
const bool uavos::stream_webrtc::CVideoRecording::saveImageinPNG() const
{
    Json_de jsonConfig = CConfigFile::getInstance().GetConfigJSON();
    if (jsonConfig.contains("media_image_png") == false) 
    {
        return true;
    }
    
    const bool use_jpeg = jsonConfig["media_image_png"].get<bool>();
   
    return use_jpeg;
}

/**
 * @brief reads "send_image_gcs" in config file.
 * default: true
 * @return true 
 * @return false 
 */
const bool uavos::stream_webrtc::CVideoRecording::sendImageToGCS() const
{
    Json_de jsonConfig = CConfigFile::getInstance().GetConfigJSON();
    if (jsonConfig.contains("send_image_gcs") == false) 
    {
        return true;
    }
    
    const bool use_jpeg = jsonConfig["send_image_gcs"].get<bool>();
   
    return use_jpeg;
}



bool uavos::stream_webrtc::CVideoRecording::startRecording()
{
    m_timer_video.reset();
    stopRecording();
    

    webrtc::MutexLock lock(&m_lock_video); // should be after stop recording as stoprecording uses same lock.

    std::time_t time_stamp;
    time_stamp = std::time(nullptr);
    m_video_file_name = getMediaFolderPath() + "v_" + uavos::util::CHelper::getFileTimeStamp() + ".y4m";
    m_video_handler = fopen(m_video_file_name.c_str(), "wb");
    
    m_video_file_header_written = false;
    m_is_video_recording = true;

    return true;
}
    

bool uavos::stream_webrtc::CVideoRecording::stopRecording()
{
    webrtc::MutexLock lock(&m_lock_video);

    if (m_video_handler != nullptr)
    {
        fclose(m_video_handler);
        m_video_handler= nullptr;
    }

    m_is_video_recording = false;
    m_video_file_header_written = false;
    
    return true;
}



bool uavos::stream_webrtc::CVideoRecording::takeImage(const uint &image_count, const uint &image_duration, uavos::stream_webrtc::CRecorderEvents * recorder_events)
{
    webrtc::MutexLock lock(&m_lock_image);
    m_recorder_events = recorder_events;
    m_image_count     = image_count;
    m_image_duration  = image_duration * 1000; // convert to ms
    m_timer_image.reset();
    return true;
}


int uavos::stream_webrtc::CVideoRecording::printPlane(const uint8_t* buf,
               const int& width,
               const int& height,
               const int& stride) {
    for (int i = 0; i < height; i++, buf += stride) {
        if (fwrite(buf, 1, width, m_video_handler) != static_cast<unsigned int>(width))
        return -1;
    }
    return 0;
}


/**
 * @brief Save a video frame in a file.
 * 
 * @param frame 
 * @return int 
 */
int uavos::stream_webrtc::CVideoRecording::printVideoFrame(const webrtc::VideoFrame& frame) 
{
    
    if (m_timer_video.elapsed_milli() < m_frame_duration) return 0;

    m_timer_video.reset();
    
    webrtc::MutexLock lock(&m_lock_video);
    if (!m_is_video_recording) return 0;

    if (m_video_handler == nullptr) return -1;
    
    webrtc::I420BufferInterface &frame_I420 = *frame.video_frame_buffer()->ToI420();
    const int width = frame_I420.width();
    const int height = frame_I420.height();
    const int chroma_width = frame_I420.ChromaWidth();
    const int chroma_height = frame_I420.ChromaHeight();

    if (!m_video_file_header_written)
    {   
        
        std::string helper_file = m_video_file_name + ".hlp";
        FILE *fp = fopen(helper_file.c_str(), "wb");
        fprintf(fp,"vlc --demux rawvideo --rawvid-fps %d  --rawvid-width %d --rawvid-height %d --rawvid-chroma  I420 %s \n",m_fps, width, height, m_video_file_name.c_str());
        fclose(fp);

        fprintf(m_video_handler, "YUV4MPEG2 W%d H%d F%d:1 C420\n", width, height,
          m_fps);
        

        m_video_file_header_written = true;
    }

    fprintf(m_video_handler, "FRAME\n");
    if (printPlane(frame_I420.DataY(), width, height, frame_I420.StrideY()) < 0) {
        return -1;
    }
    if (printPlane(frame_I420.DataU(), chroma_width, chroma_height, frame_I420.StrideU()) < 0) {
        return -1;
    }
    if (printPlane(frame_I420.DataV(), chroma_width, chroma_height, frame_I420.StrideV()) < 0) {
        return -1;
    }
    fflush(m_video_handler);
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
    infoHeader[14] = (unsigned char)(BYTES_PER_PIXEL_RGB24*8);

    return infoHeader;
}



int uavos::stream_webrtc::CVideoRecording::saveFrameAsPNG(webrtc::VideoFrame& frame)
{
    if (!saveImageinPNG()) return 0;

    webrtc::MutexLock lock(&m_lock_image);

    if (m_image_count <= 0)
    {
        return 0;
    }

    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: m_image_count: " << std::to_string(m_image_count) << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    if ((m_image_duration != 0) && (m_timer_image.elapsed_milli() < m_image_duration))
    {
        // too soon. wait more time.
        return 0;
    }
    m_timer_image.reset();

    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: m_image_duration: " << std::to_string(m_image_duration) << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    m_image_count--;

    // convert to RGB
    size_t file_size = frame.width() * frame.height() * 3; //BYTES_PER_PIXEL_RGBA;
    std::unique_ptr<uint8_t[]> res_rgb_buffer(new uint8_t[file_size]);
    webrtc::ConvertFromI420(frame, webrtc::VideoType::kRGB24, 0,
                                    res_rgb_buffer.get()); 

    // ! BUG: for unknown reason the TGB is translated by PNG lib as BGR.
    // ! so I had to swap it.
    // ! maybe related to https://forum.lvgl.io/t/png-decoding-why-red-and-blue-are-swapped/72/5
    for (size_t i = 0; i < file_size; i += 3) {
        uint8_t temp = res_rgb_buffer[i];
        res_rgb_buffer[i] = res_rgb_buffer[i + 2]; // Swap red and blue
        res_rgb_buffer[i + 2] = temp;
        }

    // choose file name
    std::string output_file_name = getMediaFolderPath() + "img_" + uavos::util::CHelper::getFileTimeStamp() + "_" + std::to_string(m_image_count) 
                                    + uavos::CWEBRTC_Plugin::getInstance().getLocationInfoText() + ".png";
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "PNG" << _NORMAL_CONSOLE_TEXT_ << std::endl;

    // convert RGB buffer to PNG image
    std::vector<unsigned char> png_image;
    unsigned error = lodepng::encode(png_image, res_rgb_buffer.get(), frame.width(), frame.height(), LCT_RGB);
    if (error)
    {
        // handle encoding error
        std::cout << "Error encoding PNG: " << lodepng_error_text(error) << std::endl;
        return 0;
    }

    // save PNG image to file
    error = lodepng::save_file(png_image, output_file_name);
    if (error)
    {
        // handle file saving error
        std::cout << "Error saving PNG file: " << lodepng_error_text(error) << std::endl;
        return 0;
    }

    std::thread t = std::thread{ [&, output_file_name]() {
        std::string file_name = output_file_name;
        if (m_recorder_events != nullptr)
        {
            m_recorder_events->onImageRecorded(file_name, sendImageToGCS());
            return 0;
        }
        return 0;
    }};

    t.detach();
    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "PNG3" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    return 0;
}


int uavos::stream_webrtc::CVideoRecording::saveFrameAsJPG(webrtc::VideoFrame& frame)
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
    size_t file_size = frame.width() * frame.height() * BYTES_PER_PIXEL_RGB24;
    std::unique_ptr<uint8_t[]> res_rgb_buffer(new uint8_t[file_size]);
    
    // convert to RGB
    webrtc::ConvertFromI420(frame, webrtc::VideoType::kRGB24, 0,
                                    res_rgb_buffer.get());
    

    std::string location_text = uavos::CWEBRTC_Plugin::getInstance().getLocationInfoText();

    // choose file name
    std::string output_file_name =  getMediaFolderPath() + "img_" + uavos::util::CHelper::getFileTimeStamp() + "_" + std::to_string(m_image_count) + location_text + ".jpg";
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    
    // open file
    FILE* image_handler = fopen(output_file_name.c_str(), "wb");
    
    // Invoking LIBJPEG
    const int quality = 90;
    const int kColorPlanes = 3;  // R, G and B.

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG-01" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    cinfo.err = jpeg_std_error(&jerr);
    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG-001:" << cinfo.err << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    jpeg_create_compress(&cinfo);
    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG-1" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    jpeg_stdio_dest(&cinfo, image_handler);
    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG-2" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    cinfo.image_width = frame.width();
    cinfo.image_height = frame.height();
    cinfo.input_components = kColorPlanes;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG-3" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif

    jpeg_start_compress(&cinfo, TRUE);
    int row_stride = frame.width() * kColorPlanes;
    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG-4" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &res_rgb_buffer.get()[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG2" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(image_handler);

    std::thread t =std::thread {[&, output_file_name](){ 
        std::string file_name = output_file_name;
        if (m_recorder_events!= nullptr)
        {
            m_recorder_events->onImageRecorded(file_name, sendImageToGCS());
            return 0;
        }
        return 0;
    }};


    t.detach();
    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG3" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    return 0;
}



/**
 * @brief Saves a frame as a BMP file.
 * 
 * @param frame 
 * @return int 
 */
int  uavos::stream_webrtc::CVideoRecording::saveFrameAsRGB( webrtc::VideoFrame& frame)
{
    // dont save twice.
    if (saveImageinPNG()) return 0;
    
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
    size_t file_size = frame.width() * frame.height() * BYTES_PER_PIXEL_RGB24;
    std::unique_ptr<uint8_t[]> res_rgb_buffer(new uint8_t[file_size]);
    
    // convert to RGB
    webrtc::ConvertFromI420(frame, webrtc::VideoType::kRGB24, 0,
                                    res_rgb_buffer.get());
    
    
    // choose file name
    std::string output_file_name =  getMediaFolderPath() + "img_" + uavos::util::CHelper::getFileTimeStamp() + "_" + std::to_string(m_image_count) 
                                + uavos::CWEBRTC_Plugin::getInstance().getLocationInfoText() + ".bmp";
    
    // open file
    FILE* image_handler = fopen(output_file_name.c_str(), "wb");

    // create BMP Header
    int widthInBytes = frame.width() * BYTES_PER_PIXEL_RGB24;
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

    std::thread t =std::thread {[&, output_file_name](){ 
        std::string file_name = output_file_name;
        if (m_recorder_events!= nullptr)
        {
            m_recorder_events->onImageRecorded(file_name, sendImageToGCS());
            return 0;
        }
        return 0;
    }};

    t.detach();
    return 0;
}