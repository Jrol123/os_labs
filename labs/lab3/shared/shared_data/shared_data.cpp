#include "shared_data.hpp"
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#include <signal.h>
#endif

namespace cplib
{

    void SharedDataManager::Lock()
    {
        shared_mem_.Lock();
    }
    void SharedDataManager::Unlock()
    {
        shared_mem_.Unlock();
    }

    SharedDataManager::SharedDataManager(const std::string &name)
        : shared_mem_(name.c_str())
    {
    }

    long long SharedDataManager::getCurrentTimestamp()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    void SharedDataManager::registerConnection()
    {
        if (!isValid())
            return;

        Lock();
        // Регистрация времени подключения
        if (shared_mem_.Data()->connection_count < 10)
        {
            shared_mem_.Data()->connection_timestamps[shared_mem_.Data()->connection_count] = getCurrentTimestamp();
            shared_mem_.Data()->connection_count++;
        }
        Unlock();
    }

    bool SharedDataManager::checkMasterAlive()
    {
        if (!isValid())
            return false;

        Lock();
        int master_pid = shared_mem_.Data()->master_pid;
        Unlock();

        if (master_pid == 0)
            return false;

        // Проверяем, жив ли мастер
#if defined(_WIN32)
        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, master_pid);
        if (process)
        {
            DWORD exit_code;
            GetExitCodeProcess(process, &exit_code);
            CloseHandle(process);
            return (exit_code == STILL_ACTIVE);
        }
        return false;
#else
        return (kill(master_pid, 0) == 0);
#endif
    }

    void SharedDataManager::becomeMaster()
    {
        if (!isValid())
            return;

        Lock();
        shared_mem_.Data()->master_pid =
#if defined(_WIN32)
            GetCurrentProcessId();
#else
            getpid();
#endif
        shared_mem_.Data()->master_timestamp = getCurrentTimestamp();
        Unlock();
    }

    bool SharedDataManager::isMaster()
    {
        if (!isValid())
            return false;

        // Блокируем для чтения master_pid
        Lock();
        int master_pid = shared_mem_.Data()->master_pid;
        Unlock();

        // Если master_pid = 0, значит мастер еще не назначен
        if (master_pid == 0)
        {
            Lock();
            shared_mem_.Data()->master_pid =
#if defined(_WIN32)
                GetCurrentProcessId();
#else
                getpid();
#endif
            Unlock();
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
        Lock();
        shared_mem_.Data()->master_pid = GetCurrentProcessId();
        Unlock();
        return true;
#else
        if (kill(master_pid, 0) == 0)
        {
            return false; // Мастер жив
        }
        // Мастер умер, становимся мастером
        Lock();
        shared_mem_.Data()->master_pid = getpid();
        Unlock();
        return true;
#endif
    }

    int SharedDataManager::getCounter()
    {
        Lock();
        int value = shared_mem_.Data()->counter;
        Unlock();
        return value;
    }

    void SharedDataManager::setCounter(int value)
    {
        Lock();
        shared_mem_.Data()->counter = value;
        Unlock();
    }

    void SharedDataManager::incrementCounter()
    {
        Lock();
        shared_mem_.Data()->counter++;
        Unlock();
    }

    bool SharedDataManager::checkProcessAlive(int pid)
    {
        if (pid == 0)
            return false;

#if defined(_WIN32)
        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (process)
        {
            DWORD exit_code;
            GetExitCodeProcess(process, &exit_code);
            CloseHandle(process);
            return (exit_code == STILL_ACTIVE);
        }
        return false;
#else
        // Проверка на существование процесса, без его убийства
        return (kill(pid, 0) == 0);
#endif
    }

    int SharedDataManager::getChild1Pid()
    {
        shared_mem_.Lock();
        int pid = shared_mem_.Data()->child1_pid;
        shared_mem_.Unlock();
        return pid;
    }

    void SharedDataManager::setChild1Pid(int pid)
    {
        shared_mem_.Lock();
        shared_mem_.Data()->child1_pid = pid;
        shared_mem_.Unlock();
    }

    int SharedDataManager::getChild2Pid()
    {
        shared_mem_.Lock();
        int pid = shared_mem_.Data()->child2_pid;
        shared_mem_.Unlock();
        return pid;
    }

    void SharedDataManager::setChild2Pid(int pid)
    {
        shared_mem_.Lock();
        shared_mem_.Data()->child2_pid = pid;
        shared_mem_.Unlock();
    }

}