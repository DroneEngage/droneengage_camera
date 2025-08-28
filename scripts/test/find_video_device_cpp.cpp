#include <iostream>      // For input/output operations (e.g., std::cout, std::cerr)
#include <string>        // For string manipulation (e.g., std::string)
#include <fstream>       // For file stream operations (e.g., std::ifstream)
#include <filesystem>    // For directory iteration and path manipulation (C++17+)
#include <algorithm>     // For std::remove_if (used for trimming whitespace)
#include <stdexcept>     // For std::runtime_error

namespace fs = std::filesystem; // Use a shorter alias for std::filesystem

//g++ -std=c++17 -o find_device find_video_device_cpp.cpp -lstdc++fs

// Function to trim leading and trailing whitespace from a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (std::string::npos == first) {
        return str; // No non-whitespace characters
    }
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}

/**
 * @brief Finds the index of a video device given its name.
 *
 * This function iterates through /sys/devices/virtual/video4linux/ directories,
 * reads the 'name' or 'card' file within each, and compares it to the
 * provided target device name.
 *
 * @param targetDeviceName The name of the video device to search for.
 * @return The integer index of the device (e.g., 0 for /dev/video0),
 * or -1 if the device is not found.
 */
int findVideoDeviceIndex(const std::string& targetDeviceName) {
    std::cout << "Searching for virtual camera: " << targetDeviceName << std::endl;

    // Define the base path where video4linux devices are listed in sysfs
    fs::path sysfsPath("/sys/devices/virtual/video4linux/");

    // Check if the directory exists and is accessible
    if (!fs::exists(sysfsPath) || !fs::is_directory(sysfsPath)) {
        std::cerr << "Error: " << sysfsPath << " does not exist or is not a directory." << std::endl;
        return -1;
    }

    try {
        // Iterate through all entries in the video4linux directory
        for (const auto& entry : fs::directory_iterator(sysfsPath)) {
            // Check if the current entry is a directory and its name starts with "video"
            if (entry.is_directory() && entry.path().filename().string().rfind("video", 0) == 0) {
                std::string currentCardLabel;
                fs::path deviceDir = entry.path(); // Get the path to the videoX directory

                // Try to read 'name' file first (more common on recent kernels for v4l2loopback)
                fs::path nameFilePath = deviceDir / "name";
                std::ifstream nameFile(nameFilePath);
                if (nameFile.is_open()) {
                    std::getline(nameFile, currentCardLabel);
                    nameFile.close();
                } else {
                    // Fallback to 'card' file for older kernels
                    fs::path cardFilePath = deviceDir / "card";
                    std::ifstream cardFile(cardFilePath);
                    if (cardFile.is_open()) {
                        std::getline(cardFile, currentCardLabel);
                        cardFile.close();
                    }
                }

                // Debugging: Uncomment the line below to see all device labels being checked
                // std::cout << "Checking device " << deviceDir.filename().string()
                //           << " with label: '" << currentCardLabel << "'" << std::endl;

                // Check if the current device's label matches our target, ignoring leading/trailing whitespace
                if (trim(currentCardLabel) == trim(targetDeviceName)) {
                    // Extract the device number from the directory name (e.g., "video0" -> "0")
                    std::string deviceNumberStr = deviceDir.filename().string().substr(5); // Remove "video" prefix
                    try {
                        int deviceNumber = std::stoi(deviceNumberStr); // Convert string to integer
                        std::cout << "Found target device: /dev/video" << deviceNumber << std::endl;
                        return deviceNumber; // Found our target, return the index
                    } catch (const std::invalid_argument& e) {
                        std::cerr << "Error: Could not convert '" << deviceNumberStr << "' to integer: " << e.what() << std::endl;
                    } catch (const std::out_of_range& e) {
                        std::cerr << "Error: Device number '" << deviceNumberStr << "' out of range: " << e.what() << std::endl;
                    }
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        // Catch filesystem-related errors
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return -1;
    }

    std::cout << "Virtual camera '" << targetDeviceName << "' not found." << std::endl;
    return -1; // Device not found
}

int main() {
    // Example usage:
    // Replace "My Virtual Camera" with the actual name of your virtual camera
    // You can also try "v4l2loopback" which is a common default name.
    std::string targetCamName = "DE-CAM1"; // Changed to "DE-CAM1" as per your request
    // Or, for a common default:
    // std::string targetCamName = "v4l2loopback";

    int deviceIndex = findVideoDeviceIndex(targetCamName);

    if (deviceIndex != -1) {
        std::cout << "Device found at /dev/video" << deviceIndex << std::endl;
    } else {
        std::cout << "Device not found." << std::endl;
    }

    // Another example: searching for a common webcam label
    std::cout << "\nAttempting to find a 'USB Camera'..." << std::endl;
    int usbCamIndex = findVideoDeviceIndex("USB Camera");
    if (usbCamIndex != -1) {
        std::cout << "USB Camera found at /dev/video" << usbCamIndex << std::endl;
    } else {
        std::cout << "USB Camera not found." << std::endl;
    }


    return 0; // Indicate successful execution
}
