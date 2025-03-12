#ifndef LOGGING_H
#define LOGGING_H

#include <string>
#include <vector>
#include <mutex>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace runtime {

    enum class LogLevel : int{
        NONE    = 0,  // No logging output
        ERROR   = 1,  // Critical errors that prevent program execution
        WARNING = 2,  // Non-critical issues that might indicate problems
        INFO    = 3,  // General information about program execution
        DEBUG   = 4,  // Detailed information for debugging purposes
        COUNT        // Used to determine number of log levels
    };

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLevel(LogLevel level) {
        m_level = level;
    }
    
    void setConsoleOutput(bool enabled) {
        m_logToConsole = enabled;
    }
    
    template<typename... Args>
    void log(LogLevel level, const char* format, Args... args) {
        if (level > m_level) return;
        
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), format, args...);
        logMessage(level, buffer);
    }
    
    void error(const char* message) { log(LogLevel::ERROR, "%s", message); }
    void warning(const char* message) { log(LogLevel::WARNING, "%s", message); }
    void info(const char* message) { log(LogLevel::INFO, "%s", message); }
    void debug(const char* message) { log(LogLevel::DEBUG, "%s", message); }
    
    const std::vector<std::string>& getMessages() const {
        return m_logMessages;
    }
    
    void clearMessages() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_logMessages.clear();
    }

private:
    Logger() = default;
    ~Logger() = default;
    
    // Delete copy and move constructors
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;
    
    void logMessage(enum class LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] ";
        
        switch (level) {
            case LogLevel::ERROR: ss << "[ERROR] "; break;
            case LogLevel::WARNING: ss << "[WARNING] "; break;
            case LogLevel::INFO: ss << "[INFO] "; break;
            case LogLevel::DEBUG: ss << "[DEBUG] "; break;
            default: break;
        }
        
        ss << message;
        m_logMessages.push_back(ss.str());
        
        if (m_logToConsole) {
            std::cout << ss.str() << std::endl;
        }
    }

    enum class LogLevel  m_level {enum class LogLevel::INFO};
    bool m_logToConsole{true};
    std::vector<std::string> m_logMessages;
    std::mutex m_mutex;
};

#define LOG_ERROR(msg, ...) runtime::Logger::getInstance().log(runtime::LogLevel::ERROR, msg, ##__VA_ARGS__)
#define LOG_WARNING(msg, ...) runtime::Logger::getInstance().log(runtime::LogLevel::WARNING, msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...) runtime::Logger::getInstance().log(runtime::LogLevel::INFO, msg, ##__VA_ARGS__)
#define LOG_DEBUG(msg, ...) runtime::Logger::getInstance().log(runtime::LogLevel::DEBUG, msg, ##__VA_ARGS__)

} // namespace runtime


#endif // LOGGING_H