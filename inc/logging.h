#pragma once
#include <vulkan/vulkan.h>
#include <volk.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <chrono>
#include <memory>

namespace runtime {
    enum class LogLevel {
        NONE,
        INFO,
        WARNING,
        DEBUG,
        ERROR
    };

    class Logger {
        LogLevel level = LogLevel::NONE;
        bool log_out_to_console = true;

        
        void _log(const char* message) {
            log_messages.push_back(message);

            if (log_out_to_console) {
                printf("%s\n", message);
            }
        }

    public:
        Logger() = default;
        ~Logger() = default;
        
        void log(LogLevel status, const char* message) {
            _log(message);
        }

        std::vector<std::string> log_messages;
    };

    static Logger logger;
}