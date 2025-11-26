#ifndef DATABASE_H
#define DATABASE_H

#include "common.h"
#include <sqlite3.h>
#include <memory>
#include <mutex>

class Database
{
public:
    static Database &getInstance();
    bool initialize(const std::string &db_path = "data/temperature_data.db");
    void shutdown();

    bool logTemperature(double temperature, const common::TimePoint &timestamp);
    std::vector<std::pair<common::TimePoint, double>> getRecentMeasurements(int count);
    std::vector<std::pair<common::TimePoint, double>> getMeasurementsInRange(
        const common::TimePoint &start, const common::TimePoint &end);
    double getAverageInRange(const common::TimePoint &start, const common::TimePoint &end);

private:
    Database() = default;
    ~Database();

    bool createTables();
    std::string timePointToSQLiteString(const common::TimePoint &time);

    sqlite3 *db_ = nullptr;
    std::mutex db_mutex_;
    bool initialized_ = false;
};

#endif