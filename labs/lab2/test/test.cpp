#include "background_launcher.h"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <program> [args...]\n";
        return 1;
    }

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    try {
        auto handle = BackgroundLauncher::launch(args);
        std::cout << "Process started successfully\n";

        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        int exit_code;
        if (BackgroundLauncher::wait(handle, &exit_code)) {
            std::cout << "Process exited with code: " << exit_code << "\n";
        } else {
            std::cerr << "Failed to wait for process\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    std::cout << "GG WP" << std::endl;

    return 0;
}