#pragma once

#include <string>
#include <mutex>
#include <fstream> // Include this to define std::ofstream
#include "defines.h"

// Log level
enum LogLevel {
    DEBUG,
    INFO,
    WARNING,
    LOG_ERROR
};

class Logger {
private:
    std::ofstream log_stream;
    LogLevel current_log_level = INFO; // Default log level
    std::string log_file = "log.txt"; // Default log file name
    std::string log_file_path = LOG_FILE_PATH; // Default log file path

    bool use_file_logging = true; // Flag to indicate if file logging is enabled

    std::mutex log_mutex; // Mutex for thread safety

    // ANSI color codes
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string YELLOW = "\033[33m";
    const std::string GREEN = "\033[32m";
    const std::string CYAN = "\033[36m";

    Logger();
    ~Logger();

public:
    static Logger& logger();

    Logger(const Logger&) = delete; // Prevent copying
    Logger& operator=(const Logger&) = delete; // Prevent assignment

    void set_log_level(LogLevel level);

    // Logging functions
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void message(LogLevel level, const std::string& message);
};