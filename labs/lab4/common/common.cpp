#include "common.h"
#include <iomanip>
#include <sstream>
#include <iostream>

#ifdef _WIN32
#include <direct.h>
#endif

namespace common {

std::string timeToString(const TimePoint& time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string timeToFileName(const TimePoint& time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

bool createDirectory(const std::string& path) {
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

std::string getLogDirectory() {
    return "logs";
}

bool fileExists(const std::string& path) {
#ifdef _WIN32
    DWORD attrib = GetFileAttributesA(path.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
#endif
}

} // namespace common