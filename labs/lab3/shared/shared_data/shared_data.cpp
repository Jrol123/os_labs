#include "shared_data.hpp"
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace cplib
{

    SharedDataManager::SharedDataManager(const std::string &name)
        : shared_mem_(name.c_str())
    {
    }

    bool SharedDataManager::isMaster()
    {
        if (!isValid())
            return false;

        // Блокируем для чтения master_pid
        shared_mem_.Lock();
        int master_pid = shared_mem_.Data()->master_pid;
        shared_mem_.Unlock();

        // Если master_pid = 0, значит мастер еще не назначен
        if (master_pid == 0)
        {
            shared_mem_.Lock();
            shared_mem_.Data()->master_pid =
#if defined(_WIN32)
                GetCurrentProcessId();
#else
                getpid();
#endif
            shared_mem_.Unlock();
            return true;
        }

        // Проверяем, жив ли мастер
#if defined(_WIN32)
        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, master_pid);
        if (process)
        {
            DWORD exit_code;
            GetExitCodeProcess(process, &exit_code);
            CloseHandle(process);
            if (exit_code == STILL_ACTIVE)
            {
                return false; // Мастер жив, мы не мастер
            }
        }
        // Мастер умер, становимся мастером
        shared_mem_.Lock();
        shared_mem_.Data()->master_pid = GetCurrentProcessId();
        shared_mem_.Unlock();
        return true;
#else
        // В Linux проверяем существование процесса через kill 0
        if (kill(master_pid, 0) == 0)
        {
            return false; // Мастер жив
        }
        // Мастер умер, становимся мастером
        shared_mem_.Lock();
        shared_mem_.Data()->master_pid = getpid();
        shared_mem_.Unlock();
        return true;
#endif
    }

    int SharedDataManager::getCounter()
    {
        shared_mem_.Lock();
        int value = shared_mem_.Data()->counter;
        shared_mem_.Unlock();
        return value;
    }

    void SharedDataManager::setCounter(int value)
    {
        shared_mem_.Lock();
        shared_mem_.Data()->counter = value;
        shared_mem_.Unlock();
    }

    void SharedDataManager::incrementCounter()
    {
        shared_mem_.Lock();
        shared_mem_.Data()->counter++;
        shared_mem_.Unlock();
    }

} // namespace cplib