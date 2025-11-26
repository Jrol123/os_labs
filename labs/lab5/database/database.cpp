#include "database.h"
#include <iostream>
#include <sstream>
#include <iomanip>

Database &Database::getInstance()
{
    static Database instance;
    return instance;
}

bool Database::initialize(const std::string &db_path)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (initialized_)
    {
        return true;
    }

    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    if (!createTables())
    {
        std::cerr << "Failed to create tables" << std::endl;
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }

    initialized_ = true;
    std::cout << "Database initialized successfully: " << db_path << std::endl;
    return true;
}

void Database::shutdown()
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (db_)
    {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    initialized_ = false;
}

Database::~Database()
{
    shutdown();
}

bool Database::createTables()
{
    const char *sql =
        "CREATE TABLE IF NOT EXISTS temperature_logs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "temperature REAL NOT NULL,"
        "timestamp TEXT NOT NULL,"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_timestamp ON temperature_logs(timestamp);";

    char *error_msg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &error_msg);

    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << error_msg << std::endl;
        sqlite3_free(error_msg);
        return false;
    }

    return true;
}

std::string Database::timePointToSQLiteString(const common::TimePoint &time)
{
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto tm = *std::localtime(&time_t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

bool Database::logTemperature(double temperature, const common::TimePoint &timestamp)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (!db_)
    {
        return false;
    }

    std::string timestamp_str = timePointToSQLiteString(timestamp);
    std::string sql = "INSERT INTO temperature_logs (temperature, timestamp) VALUES (?, ?);";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        return false;
    }

    sqlite3_bind_double(stmt, 1, temperature);
    sqlite3_bind_text(stmt, 2, timestamp_str.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return rc == SQLITE_DONE;
}

std::vector<std::pair<common::TimePoint, double>> Database::getRecentMeasurements(int count)
{
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<std::pair<common::TimePoint, double>> results;

    if (!db_)
    {
        return results;
    }

    std::string sql = "SELECT temperature, timestamp FROM temperature_logs ORDER BY timestamp DESC LIMIT ?;";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        return results;
    }

    sqlite3_bind_int(stmt, 1, count);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        double temp = sqlite3_column_double(stmt, 0);
        const char *timestamp_str = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

        std::tm tm = {};
        std::istringstream ss(timestamp_str);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

        if (!ss.fail())
        {
            auto time_point = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            results.emplace_back(time_point, temp);
        }
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<std::pair<common::TimePoint, double>> Database::getMeasurementsInRange(
    const common::TimePoint &start, const common::TimePoint &end)
{
    std::lock_guard<std::mutex> lock(db_mutex_);
    std::vector<std::pair<common::TimePoint, double>> results;

    if (!db_)
    {
        return results;
    }

    std::string start_str = timePointToSQLiteString(start);
    std::string end_str = timePointToSQLiteString(end);

    std::string sql = "SELECT temperature, timestamp FROM temperature_logs WHERE timestamp BETWEEN ? AND ? ORDER BY timestamp;";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        return results;
    }

    sqlite3_bind_text(stmt, 1, start_str.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, end_str.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        double temp = sqlite3_column_double(stmt, 0);
        const char *timestamp_str = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));

        std::tm tm = {};
        std::istringstream ss(timestamp_str);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

        if (!ss.fail())
        {
            auto time_point = std::chrono::system_clock::from_time_t(std::mktime(&tm));
            results.emplace_back(time_point, temp);
        }
    }

    sqlite3_finalize(stmt);
    return results;
}

double Database::getAverageInRange(const common::TimePoint &start, const common::TimePoint &end)
{
    std::lock_guard<std::mutex> lock(db_mutex_);

    if (!db_)
    {
        return 0.0;
    }

    std::string start_str = timePointToSQLiteString(start);
    std::string end_str = timePointToSQLiteString(end);

    std::string sql = "SELECT AVG(temperature) FROM temperature_logs WHERE timestamp BETWEEN ? AND ?;";
    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        return 0.0;
    }

    sqlite3_bind_text(stmt, 1, start_str.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, end_str.c_str(), -1, SQLITE_STATIC);

    double average = 0.0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        average = sqlite3_column_double(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return average;
}