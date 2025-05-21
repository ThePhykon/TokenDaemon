#include "logger.hpp"

#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip> // For std::put_time

Logger::Logger() {
    // Open the log file in append mode
    if(!use_file_logging){
        return;
    }

    log_stream.open(log_file_path + log_file, std::ios::app);
    if (!log_stream.is_open()) {
        std::cerr << "Failed to open log file: " << log_file << std::endl;
    }
}

Logger::~Logger() {
    if (log_stream.is_open()) {
        log_stream.close(); // Close the log file
    }
}

Logger& Logger::logger() {
    static Logger instance; // Create a static instance of Logger
    return instance; // Return the instance
}

void Logger::set_log_level(LogLevel level) {
    std::lock_guard<std::mutex> lock(log_mutex); // Lock the mutex for thread safety
    current_log_level = level; // Set the current log level
}

// Helper function to get the current timestamp
std::string get_current_timestamp() {
    auto now = std::time(nullptr);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    oss << " UTC"; // Append "UTC" to the timestamp
    return oss.str();
}

// Logging functions
void Logger::debug(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex); // Lock the mutex for thread safety
    if (current_log_level <= DEBUG) {
        std::string timestamp = get_current_timestamp();
        std::cout << CYAN << "[" << timestamp << "] [DEBUG] " << RESET << message << std::endl; // Log debug messages in cyan

        if(log_stream.is_open()) {
            log_stream << "[" << timestamp << "] [DEBUG] " << message << std::endl; // Log to file
        }
    }
}

void Logger::info(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex); // Lock the mutex for thread safety
    if (current_log_level <= INFO) {
        std::string timestamp = get_current_timestamp();
        std::cout << GREEN << "[" << timestamp << "] [INFO] " << RESET << message << std::endl; // Log info messages in green

        if(log_stream.is_open()) {
            log_stream << "[" << timestamp << "] [INFO] " << message << std::endl; // Log to file
        }
    }
}

void Logger::warning(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex); // Lock the mutex for thread safety
    if (current_log_level <= WARNING) {
        std::string timestamp = get_current_timestamp();
        std::cout << YELLOW << "[" << timestamp << "] [WARNING] " << RESET << message << std::endl; // Log warning messages in yellow
        
        if(log_stream.is_open()) {
            log_stream << "[" << timestamp << "] [WARNING] " << message << std::endl; // Log to file
        }
    }
}

void Logger::error(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex); // Lock the mutex for thread safety
    if (current_log_level <= LOG_ERROR) {
        std::string timestamp = get_current_timestamp();
        std::cerr << RED << "[" << timestamp << "] [ERROR] " << RESET << message << std::endl; // Log error messages in red
        
        if(log_stream.is_open()) {
            log_stream << "[" << timestamp << "] [ERROR] " << message << std::endl; // Log to file
        }
    }
}

void Logger::message(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex); // Lock the mutex for thread safety
    switch (level) {
        case DEBUG:
            debug(message); // Log debug message
            break;
        case INFO:
            info(message); // Log info message
            break;
        case WARNING:
            warning(message); // Log warning message
            break;
        case LOG_ERROR:
            error(message); // Log error message
            break;
    }
}