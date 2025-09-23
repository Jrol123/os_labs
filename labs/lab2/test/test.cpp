#include "background_launcher.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

int main(int argc, char *argv[])
{
    // setlocale(LC_ALL, "Russian");
    // SetConsoleCP(1251);
    // SetConsoleOutputCP(1251);
    if (argc < 2)
    {
        cout << "Usage: " << argv[0] << " <program> [args...]" << endl;
        return 1;
    }

    vector<string> args;
    for (int i = 1; i < argc; ++i)
    {
        args.push_back(argv[i]);
    }

    try
    {
        auto handle = BackgroundLauncher::launch(args);
        cout << "Process started successfully" << endl;

        this_thread::sleep_for(chrono::seconds(1));

        int exit_code;
        if (BackgroundLauncher::wait(handle, &exit_code))
        {
            cout << "Process exited with code: " << exit_code << endl;
        }
        else
        {
            cerr << "Failed to wait for process" << endl;
            return 1;
        }
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}