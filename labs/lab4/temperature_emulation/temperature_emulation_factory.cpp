#include "temperature_emulation.h"
#include <memory>

std::shared_ptr<TemperatureEmulator> createTemperatureEmulator(
    double base_temp, double amplitude, double noise_level)
{
    return std::make_shared<TemperatureEmulator>(base_temp, amplitude, noise_level);
}