#include "background_launcher.h"
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <shellapi.h>

using namespace std;

ProcessHandle BackgroundLauncher::launch(const vector<string> &args)
{
    string command_line;
    for (const auto &arg : args)
    {
        if (!command_line.empty())
            command_line += " ";
        command_line += arg;
    }

    PROCESS_INFORMATION pi;
    STARTUPINFOA si = {sizeof(si)};

    if (!CreateProcessA(
            nullptr,
            command_line.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi))
    {
        throw runtime_error("CreateProcess failed");
    }

    CloseHandle(pi.hThread);
    return pi.hProcess;
}

bool BackgroundLauncher::wait(ProcessHandle handle, int *exit_code)
{
    DWORD result = WaitForSingleObject(handle, INFINITE);
    if (result != WAIT_OBJECT_0)
        return false;

    if (exit_code)
    {
        DWORD code;
        if (!GetExitCodeProcess(handle, &code))
            return false;
        *exit_code = static_cast<int>(code);
    }

    CloseHandle(handle);
    return true;
}

#else
#include <unistd.h>
#include <sys/wait.h>

ProcessHandle BackgroundLauncher::launch(const vector<string> &args)
{
    vector<char *> argv;
    for (const auto &arg : args)
    {
        argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork(); // Делаем дочерний процесс
    if (pid == -1)
        throw runtime_error("fork failed");

    if (pid == 0)
    {
        // Сам дочерний процесс
        execvp(argv[0], argv.data());
        exit(EXIT_FAILURE);
    }

    return pid;
}

bool BackgroundLauncher::wait(ProcessHandle handle, int *exit_code)
{
    int status;
    if (waitpid(handle, &status, 0) == -1)
        return false;

    if (exit_code)
    {
        if (WIFEXITED(status)) // Завершился ли нормально или произошла авария
            *exit_code = WEXITSTATUS(status);
        else
            return false;
    }
    return true;
}
#endif