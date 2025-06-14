#ifndef GUI_H_
#define GUI_H_


// Structure to hold YUV color components
struct YuvColor {
    uint8_t y;
    uint8_t u;
    uint8_t v;
};

// Function to convert RGB to YUV (ITU-R BT.601 standard for YCbCr, suitable for YUV)
// Values are clamped to 0-255
YuvColor RgbToYuv(uint8_t r, uint8_t g, uint8_t b) {
    YuvColor yuv;
    // Y (Luma) calculation
    yuv.y = static_cast<uint8_t>(std::clamp((0.257 * r) + (0.504 * g) + (0.098 * b) + 16, 0.0, 255.0));
    // U (Cb) calculation
    yuv.u = static_cast<uint8_t>(std::clamp((-0.148 * r) - (0.291 * g) + (0.439 * b) + 128, 0.0, 255.0));
    // V (Cr) calculation
    yuv.v = static_cast<uint8_t>(std::clamp((0.439 * r) - (0.368 * g) - (0.071 * b) + 128, 0.0, 255.0));
    return yuv;
}

#endif



