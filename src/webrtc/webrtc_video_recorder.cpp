
#include "../common.h"
#include <chrono>
#include <sys/stat.h>
#include <sys/types.h>
#include "webrtc_video_recorder.hpp"
#include <thread>         // std::thread

#include "../webrtc_plugin.hpp"
#include "../de_common/helpers/json_nlohmann.hpp"
#include "../de_common/helpers/util_rpi.hpp"
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

using namespace de;
using namespace de::stream_webrtc;

const int BYTES_PER_PIXEL_RGB24 = 3; /// red, green, & blue
const int FILE_HEADER_SIZE = 14;
const int INFO_HEADER_SIZE = 40;

const std::string de::stream_webrtc::CVideoRecording::getMediaFolderPath() const
{
    Json_de jsonConfig = CConfigFile::getInstance().GetConfigJSON();
    if (jsonConfig.contains("media_folder") == false) 
    {
        return std::string();
    }
    
    std::string file_path = jsonConfig["media_folder"].get<std::string>();

    if (file_path.empty())
    {
        return file_path;
    }

    if ((file_path.back() != '/') && (file_path.back() != '\\'))
    {
        file_path += "/";
    }

    // create folder if it does not exist (non-recursive)
    mkdir(file_path.c_str(), 0755);

    return file_path;
}

/**
 * @brief reads "media_image_png" from config file.
 * default: true.
 * 
 * @return true 
 * @return false 
 */
bool de::stream_webrtc::CVideoRecording::saveImageinPNG() const
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
bool de::stream_webrtc::CVideoRecording::sendImageToGCS() const
{
    Json_de jsonConfig = CConfigFile::getInstance().GetConfigJSON();
    if (jsonConfig.contains("send_image_gcs") == false)
    {
        return true;
    }

    const bool use_jpeg = jsonConfig["send_image_gcs"].get<bool>();

    return use_jpeg;
}


/**
 * @brief reads "video_recording_bitrate_kbps" from config file.
 * default: 2000 kbps, which is a reasonable quality/size trade-off for
 * drone downlink footage recorded at modest (720p or below) resolutions.
 *
 * @return int
 */
int de::stream_webrtc::CVideoRecording::getVideoRecordingBitrateKbps() const
{
    Json_de jsonConfig = CConfigFile::getInstance().GetConfigJSON();
    return jsonConfig.value("video_recording_bitrate_kbps", 2000);
}


/**
 * @brief reads "video_recording_fps" from config file.
 * default: 10. 0 means use the capture fps (camera.capture_fps, default 30).
 *
 * @return int
 */
int de::stream_webrtc::CVideoRecording::getVideoRecordingFps() const
{
    Json_de jsonConfig = CConfigFile::getInstance().GetConfigJSON();
    if (!jsonConfig.contains("camera") || !jsonConfig["camera"].is_object())
    {
        return 10;
    }
    const auto &cam = jsonConfig["camera"];
    const int fps = cam.value("video_recording_fps", 10);
    if (fps > 0) return fps;
    // 0 or negative: fall back to capture fps.
    return cam.value("capture_fps", 30);
}


/**
 * @brief Whether to use the Raspberry Pi V4L2 M2M hardware H.264 encoder
 * instead of software libx264. Auto-detected via device-tree model, can be
 * forced off with "video_recording_hw_encoder": false in config file (e.g.
 * if the RPi kernel/ffmpeg build does not expose h264_v4l2m2m).
 *
 * @return true
 * @return false
 */
bool de::stream_webrtc::CVideoRecording::useHardwareVideoEncoder() const
{
    Json_de jsonConfig = CConfigFile::getInstance().GetConfigJSON();
    if (jsonConfig.value("video_recording_hw_encoder", true) == false)
    {
        return false;
    }

    return helpers::CUtil_Rpi::getInstance().get_rpi_model() >= 0;
}


/**
 * @brief Builds the ffmpeg command line that turns a raw I420 stream fed on
 * stdin into a compressed H.264/MP4 file. This replaces the previous
 * approach of dumping uncompressed YUV4MPEG2 (.y4m) frames directly to disk,
 * which produced huge files and heavy I/O - impractical on a Raspberry Pi.
 *
 * @param width
 * @param height
 * @return std::string
 */
std::string de::stream_webrtc::CVideoRecording::buildFfmpegRecordCommand(const int& width, const int& height) const
{
    // single-quote the output path and escape any embedded single quotes,
    // since media_folder is user-configurable and passed through the shell.
    std::string quoted_path;
    quoted_path.reserve(m_video_file_name.size() + 2);
    quoted_path += '\'';
    for (char c : m_video_file_name)
    {
        if (c == '\'')
        {
            quoted_path += "'\\''";
        }
        else
        {
            quoted_path += c;
        }
    }
    quoted_path += '\'';

    const int bitrate_kbps = getVideoRecordingBitrateKbps();

    std::string codec_args;
    if (useHardwareVideoEncoder())
    {
        // Hardware-accelerated encode on RPi: near-zero CPU overhead.
        codec_args = "-c:v h264_v4l2m2m -b:v " + std::to_string(bitrate_kbps) + "k";
    }
    else
    {
        codec_args = "-c:v libx264 -preset ultrafast -tune zerolatency -b:v " + std::to_string(bitrate_kbps) + "k";
    }

    std::string cmd = "ffmpeg -hide_banner -loglevel error -y "
                       "-f rawvideo -pixel_format yuv420p -video_size " + std::to_string(width) + "x" + std::to_string(height) +
                       " -framerate " + std::to_string(getVideoRecordingFps()) +
                       " -i - -an " + codec_args +
                       " -pix_fmt yuv420p -movflags +faststart " + quoted_path;

    return cmd;
}


/**
 * @brief Builds a downscaled in-memory PNG for the GCS downlink.
 * @details Reads "gcs_image_small_width"/"gcs_image_small_height" from config.
 * 0 or -1 means unlimited -> returns empty (caller falls back to the saved file).
 * The locally saved still always keeps the full capture resolution; this only
 * affects what is sent over the downlink to save bandwidth on the RPi.
 *
 * @param frame Full-resolution captured frame.
 * @return std::vector<unsigned char> PNG bytes, or empty on unlimited/error.
 */
std::vector<unsigned char> de::stream_webrtc::CVideoRecording::buildSmallGCSPng(const webrtc::VideoFrame& frame)
{
    Json_de jsonConfig = CConfigFile::getInstance().GetConfigJSON();
    int target_w = 640, target_h = 480;
    if (jsonConfig.contains("camera") && jsonConfig["camera"].is_object())
    {
        const auto &cam = jsonConfig["camera"];
        target_w = cam.value("gcs_image_small_width", 640);
        target_h = cam.value("gcs_image_small_height", 480);
    }
    if (target_w <= 0 || target_h <= 0)
    {
        return std::vector<unsigned char>();
    }

    webrtc::I420BufferInterface &src_I420 = *frame.video_frame_buffer()->ToI420();
    const int src_w = src_I420.width();
    const int src_h = src_I420.height();

    // No downscale needed if already at/below target.
    if (src_w <= target_w && src_h <= target_h)
    {
        return std::vector<unsigned char>();
    }

    // Scale the I420 frame down to the target resolution.
    webrtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer =
        webrtc::I420Buffer::Create(target_w, target_h);
    scaled_buffer->ScaleFrom(src_I420);

    // Build a VideoFrame around the scaled buffer so we can reuse ConvertFromI420.
    webrtc::VideoFrame scaled_frame = webrtc::VideoFrame::Builder()
        .set_video_frame_buffer(scaled_buffer)
        .set_rotation(webrtc::kVideoRotation_0)
        .set_timestamp_us(frame.timestamp_us())
        .set_id(frame.id())
        .build();

    // Convert scaled I420 -> RGB24.
    const size_t file_size = target_w * target_h * BYTES_PER_PIXEL_RGB24;
    std::unique_ptr<uint8_t[]> res_rgb_buffer(new uint8_t[file_size]);
    webrtc::ConvertFromI420(scaled_frame, webrtc::VideoType::kRGB24, 0, res_rgb_buffer.get());

    // lodepng interprets RGB24 as BGR, so swap red/blue (same fix as saveFrameAsPNG).
    for (size_t i = 0; i < file_size; i += 3) {
        uint8_t temp = res_rgb_buffer[i];
        res_rgb_buffer[i] = res_rgb_buffer[i + 2];
        res_rgb_buffer[i + 2] = temp;
    }

    std::vector<unsigned char> png_image;
    const unsigned error = lodepng::encode(png_image, res_rgb_buffer.get(), target_w, target_h, LCT_RGB);
    if (error)
    {
        std::cout << "Error encoding small GCS PNG: " << lodepng_error_text(error) << std::endl;
        return std::vector<unsigned char>();
    }

    return png_image;
}


bool de::stream_webrtc::CVideoRecording::startRecording()
{
    m_timer_video.reset();
    stopRecording();


    webrtc::MutexLock lock(&m_lock_video); // should be after stop recording as stoprecording uses same lock.

    m_video_file_name = getMediaFolderPath() + "v_" + de::util::CHelper::getFileTimeStamp() + ".mp4";
    // ffmpeg is spawned lazily on the first frame, once we know width/height (see printVideoFrame).
    m_video_handler = nullptr;

    m_video_pipe_started = false;
    m_is_video_recording = true;

    return true;
}


bool de::stream_webrtc::CVideoRecording::stopRecording()
{
    webrtc::MutexLock lock(&m_lock_video);

    if (m_video_handler != nullptr)
    {
        // closing the pipe sends EOF to ffmpeg, which flushes the encoder and
        // finalizes the MP4 (moov atom) before exiting.
        pclose(m_video_handler);
        m_video_handler= nullptr;
    }

    m_is_video_recording = false;
    m_video_pipe_started = false;

    return true;
}



bool de::stream_webrtc::CVideoRecording::takeImage(const uint &image_count, const uint &image_duration, bool gcs_small, de::stream_webrtc::CRecorderEvents * recorder_events)
{
    webrtc::MutexLock lock(&m_lock_image);
    m_recorder_events = recorder_events;
    m_image_count     = image_count;
    m_image_duration  = image_duration * 1000; // convert to ms
    m_gcs_small       = gcs_small;
    m_timer_image.reset();
    return true;
}


int de::stream_webrtc::CVideoRecording::printPlane(const uint8_t* buf,
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
int de::stream_webrtc::CVideoRecording::printVideoFrame(const webrtc::VideoFrame& frame) 
{
    
    if (m_timer_video.elapsed_milli() < m_frame_duration) return 0;

    m_timer_video.reset();
    
    webrtc::MutexLock lock(&m_lock_video);
    if (!m_is_video_recording) return 0;

    // note: m_video_handler is nullptr until the first frame arrives and the
    // ffmpeg pipe is lazily spawned below (width/height are needed for that).

    webrtc::I420BufferInterface &frame_I420 = *frame.video_frame_buffer()->ToI420();
    const int width = frame_I420.width();
    const int height = frame_I420.height();
    const int chroma_width = frame_I420.ChromaWidth();
    const int chroma_height = frame_I420.ChromaHeight();

    if (!m_video_pipe_started)
    {
        const std::string cmd = buildFfmpegRecordCommand(width, height);
        m_video_handler = popen(cmd.c_str(), "w");
        if (m_video_handler == nullptr)
        {
            std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "Failed to start ffmpeg recorder: " << cmd << _NORMAL_CONSOLE_TEXT_ << std::endl;
            m_is_video_recording = false;
            return -1;
        }

        m_video_pipe_started = true;
    }

    // raw I420 planes are fed to ffmpeg's rawvideo demuxer back-to-back, with no framing markers.
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



unsigned char* de::stream_webrtc::CVideoRecording::createBitmapFileHeader (const uint& height, const uint& stride)
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

unsigned char* de::stream_webrtc::CVideoRecording::createBitmapInfoHeader (const uint&  height, const uint&  width)
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



int de::stream_webrtc::CVideoRecording::saveFrameAsPNG(webrtc::VideoFrame& frame)
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
    size_t file_size = frame.width() * frame.height() * BYTES_PER_PIXEL_RGB24; 
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
    std::string output_file_name = getMediaFolderPath() + "img_" + de::util::CHelper::getFileTimeStamp() + "_" + std::to_string(m_image_count) 
                                    + de::CWEBRTC_Plugin::getInstance().getLocationInfoText() + ".png";
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

    // Capture recorder_events by value to avoid race condition with detached thread
    de::stream_webrtc::CRecorderEvents* recorder_events = m_recorder_events;

    // Build the low-res GCS PNG inside the detached thread so the scale+encode
    // does not block the capture/stream pipeline. buildSmallGCSPng is static and
    // only touches the singleton config + the refcounted frame buffer, so it is
    // safe to run without `this`. The frame is captured by value to keep its
    // buffer alive for the duration of the encode.
    const bool gcs_small = m_gcs_small;
    std::thread([output_file_name, recorder_events, frame, gcs_small]() {
        std::vector<unsigned char> gcs_image;
        if (gcs_small)
        {
            gcs_image = buildSmallGCSPng(frame);
        }
        if (recorder_events != nullptr)
        {
            // sendImageToGCS() accesses singleton CConfigFile, safe to call here
            bool send_to_gcs = CConfigFile::getInstance().GetConfigJSON().value("send_image_gcs", true);
            recorder_events->onImageRecorded(output_file_name, send_to_gcs, std::move(gcs_image));
        }
    }).detach();
    #ifdef DEBUG
    std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "PNG3" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    #endif
    return 0;
}


int de::stream_webrtc::CVideoRecording::saveFrameAsJPG(webrtc::VideoFrame& frame)
{
    // ERROR IN THE NEW VERSION....
    // webrtc::MutexLock lock(&m_lock_image);

    // if (m_image_count<=0)
    // {
    //     return 0;
    // }
  
    // #ifdef DEBUG
    // std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: m_image_count: " << std::to_string(m_image_count) << _NORMAL_CONSOLE_TEXT_ << std::endl;
    // #endif  
        
    // if ((m_image_duration!=0) && (m_timer_image.elapsed_milli() < m_image_duration))
    // {
    //     // too soon. wait more time.
    //     return 0;
    // }
    // m_timer_image.reset();
    
    // #ifdef DEBUG
    // std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "DEBUG: m_image_duration: " << std::to_string(m_image_duration) <<  _NORMAL_CONSOLE_TEXT_ << std::endl;
    // #endif  
    
    // m_image_count--;

    
    // // Check https://rawpixels.net/
    // size_t file_size = frame.width() * frame.height() * BYTES_PER_PIXEL_RGB24;
    // std::unique_ptr<uint8_t[]> res_rgb_buffer(new uint8_t[file_size]);
    
    // // convert to RGB
    // webrtc::ConvertFromI420(frame, webrtc::VideoType::kRGB24, 0,
    //                                 res_rgb_buffer.get());
    

    // std::string location_text = de::CWEBRTC_Plugin::getInstance().getLocationInfoText();

    // // choose file name
    // std::string output_file_name =  getMediaFolderPath() + "img_" + de::util::CHelper::getFileTimeStamp() + "_" + std::to_string(m_image_count) + location_text + ".jpg";
    // std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    
    // // open file
    // FILE* image_handler = fopen(output_file_name.c_str(), "wb");
    
    // // Invoking LIBJPEG
    // const int quality = 90;
    // const int kColorPlanes = 3;  // R, G and B.

    // struct jpeg_compress_struct cinfo;
    // struct jpeg_error_mgr jerr;
    // JSAMPROW row_pointer[1];
    // #ifdef DEBUG
    // std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG-01" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    // #endif
    // cinfo.err = jpeg_std_error(&jerr);
    // #ifdef DEBUG
    // std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG-001:" << cinfo.err << _NORMAL_CONSOLE_TEXT_ << std::endl;
    // #endif
    // jpeg_create_compress(&cinfo);
    // #ifdef DEBUG
    // std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG-1" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    // #endif
    
    // jpeg_stdio_dest(&cinfo, image_handler);
    // #ifdef DEBUG
    // std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG-2" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    // #endif
    
    // cinfo.image_width = frame.width();
    // cinfo.image_height = frame.height();
    // cinfo.input_components = kColorPlanes;
    // cinfo.in_color_space = JCS_RGB;
    // jpeg_set_defaults(&cinfo);
    // jpeg_set_quality(&cinfo, quality, TRUE);
    // #ifdef DEBUG
    // std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG-3" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    // #endif

    // jpeg_start_compress(&cinfo, TRUE);
    // int row_stride = frame.width() * kColorPlanes;
    // #ifdef DEBUG
    // std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG-4" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    // #endif
    
    // while (cinfo.next_scanline < cinfo.image_height) {
    //     row_pointer[0] = &res_rgb_buffer.get()[cinfo.next_scanline * row_stride];
    //     jpeg_write_scanlines(&cinfo, row_pointer, 1);
    // }
    // #ifdef DEBUG
    // std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG2" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    // #endif
    

    // jpeg_finish_compress(&cinfo);
    // jpeg_destroy_compress(&cinfo);
    // fclose(image_handler);

    // std::thread t =std::thread {[&, output_file_name](){ 
    //     std::string file_name = output_file_name;
    //     if (m_recorder_events!= nullptr)
    //     {
    //         m_recorder_events->onImageRecorded(file_name, sendImageToGCS());
    //         return 0;
    //     }
    //     return 0;
    // }};


    // t.detach();
    // #ifdef DEBUG
    // std::cout << __FUNCTION__ << __LINE__ << "Key " << _ERROR_CONSOLE_BOLD_TEXT_ << "JPG3" << _NORMAL_CONSOLE_TEXT_ << std::endl;
    // #endif
    return 0;
}



/**
 * @brief Saves a frame as a BMP file.
 * 
 * @param frame 
 * @return int 
 */
int  de::stream_webrtc::CVideoRecording::saveFrameAsRGB( webrtc::VideoFrame& frame)
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
    std::string output_file_name =  getMediaFolderPath() + "img_" + de::util::CHelper::getFileTimeStamp() + "_" + std::to_string(m_image_count) 
                                + de::CWEBRTC_Plugin::getInstance().getLocationInfoText() + ".bmp";
    
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

    // Capture recorder_events by value to avoid race condition with detached thread
    de::stream_webrtc::CRecorderEvents* recorder_events = m_recorder_events;

    // Build the low-res GCS PNG inside the detached thread so the scale+encode
    // does not block the capture/stream pipeline. buildSmallGCSPng is static and
    // only touches the singleton config + the refcounted frame buffer, so it is
    // safe to run without `this`. The frame is captured by value to keep its
    // buffer alive for the duration of the encode.
    const bool gcs_small = m_gcs_small;
    std::thread([output_file_name, recorder_events, frame, gcs_small]() {
        std::vector<unsigned char> gcs_image;
        if (gcs_small)
        {
            gcs_image = buildSmallGCSPng(frame);
        }
        if (recorder_events != nullptr)
        {
            // sendImageToGCS() accesses singleton CConfigFile, safe to call here
            bool send_to_gcs = CConfigFile::getInstance().GetConfigJSON().value("send_image_gcs", true);
            recorder_events->onImageRecorded(output_file_name, send_to_gcs, std::move(gcs_image));
        }
    }).detach();

    return 0;
}