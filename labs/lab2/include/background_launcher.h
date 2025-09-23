#ifndef BACKGROUND_LAUNCHER_H
#define BACKGROUND_LAUNCHER_H

#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
using ProcessHandle = HANDLE;
#else
#include <sys/types.h>
using ProcessHandle = pid_t;
#endif

class BackgroundLauncher {
public:
    static ProcessHandle launch(const std::vector<std::string>& args);
    static bool wait(ProcessHandle handle, int* exit_code = nullptr);
};

#endif