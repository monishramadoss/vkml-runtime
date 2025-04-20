#include "logging.h"


namespace runtime {

Logger& Logger::getInstance()
{
    static Logger instance;
    return instance;
}

void Logger::setLevel(LogLevel level)
{
    m_level = level;
}

void Logger::setConsoleOutput(bool enabled)
{
    m_logToConsole = enabled;
}

void Logger::error(const char* message)
{
    log(LogLevel::ERRR, "%s", message);
}

void Logger::warning(const char* message)
{
    log(LogLevel::WARNING, "%s", message);
}

void Logger::info(const char* message)
{
    log(LogLevel::INFO, "%s", message);
}

void Logger::debug(const char* message)
{
    log(LogLevel::DEBUG, "%s", message);
}

const std::vector<std::string>& Logger::getMessages() const
{
    return m_logMessages;
}

void Logger::clearMessages()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logMessages.clear();
}


void Logger::logMessage(enum class LogLevel level, const std::string& message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] ";
    
    switch (level) {
        case LogLevel::ERRR: ss << "[ERROR] "; break;
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

} // namespace runtime