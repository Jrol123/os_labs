#ifndef TEMPERATURE_EMULATOR_H
#define TEMPERATURE_EMULATOR_H

#include "../common/common.h"
#include <functional>
#include <random>
#include <memory>

class TemperatureEmulator
{
public:
    TemperatureEmulator(double base_temp = 20.0,
                        double amplitude = 5.0,
                        double noise_level = 0.5);

    double getCurrentTemperature();
    void setBaseTemperature(double temp);
    void setAmplitude(double amplitude);
    void setNoiseLevel(double noise_level);
    void enableDailyCycle(bool enable);
    void setCustomGenerator(std::function<double()> generator);

private:
    double base_temperature_;
    double amplitude_;
    double noise_level_;
    bool daily_cycle_enabled_;

    std::minstd_rand random_engine_;
    std::normal_distribution<double> noise_distribution_;
    std::function<double()> custom_generator_;

    double generateDailyCycle();
    double generateRandomTemperature();
    double addNoise(double temperature);
};

#endif