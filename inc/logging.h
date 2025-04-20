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

    enum class LogLevel : int {
        NONE    ,  // No logging output
        ERRR   ,  // Critical errors that prevent program execution
        WARNING ,  // Non-critical issues that might indicate problems
        INFO    ,  // General information about program execution
        DEBUG   ,  // Detailed information for debugging purposes
        COUNT          // Used to determine number of log levels
    };

class Logger {
public:
    static Logger& getInstance();

    void setLevel(LogLevel level);    
    void setConsoleOutput(bool enabled);
    
    template<typename... Args>
    void log(LogLevel level, const char* format, Args... args) {
        if (level > m_level) return;
        
        char buffer[1024];
        snprintf(buffer, sizeof(buffer), format, args...);
        logMessage(level, buffer);
    }
    
    void error(const char* message);
    void warning(const char* message);
    void info(const char* message);
    void debug(const char* message);
    
    const std::vector<std::string>& getMessages() const;    
    void clearMessages();

private:
    Logger() = default;
    ~Logger() = default;
    
    // Delete copy and move constructors
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;
    
    void logMessage(LogLevel level, const std::string& message);
    LogLevel m_level {LogLevel::DEBUG};
    bool m_logToConsole = true;
    std::vector<std::string> m_logMessages;
    std::mutex m_mutex;
};

#define LOG_ERROR(msg, ...) runtime::Logger::getInstance().log(runtime::LogLevel::ERRR, msg, ##__VA_ARGS__)
#define LOG_WARNING(msg, ...) runtime::Logger::getInstance().log(runtime::LogLevel::WARNING, msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...) runtime::Logger::getInstance().log(runtime::LogLevel::INFO, msg, ##__VA_ARGS__)
#define LOG_DEBUG(msg, ...) runtime::Logger::getInstance().log(runtime::LogLevel::DEBUG, msg, ##__VA_ARGS__)

} // namespace runtime

#endif // LOGGING_H