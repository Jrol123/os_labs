#include "application.hpp"

int main(int argc, char **argv)
{
    cplib::Application app("global_counter", "process.log");
    app.run();
    return 0;
}