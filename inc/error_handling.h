#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include "logging.h"
#include <string>
#include <string_view>
#include <stdexcept>

#ifndef VOLK_HH
#define VOLK_HH
#define VK_NO_PROTOTYPES
#include <volk.h>
#endif // VOLK_HH

constexpr bool isDebugBuild = false;

namespace runtime {

/**
 * @brief Exception for Vulkan-specific errors
 */
class VulkanError : public ::std::runtime_error {
public:
    /**
     * @brief Create a new Vulkan error
     * @param result The Vulkan result code
     * @param message Error message
     */
    VulkanError(VkResult result, std::string_view message)
        : ::std::runtime_error(formatErrorMessage(result, message)), m_result(result) {
        LOG_ERROR("VulkanError: %s", what());
    }
    
    /**
     * @brief Get the Vulkan result code
     * @return The VkResult error code
     */
    [[nodiscard]] VkResult result() const noexcept { return m_result; }

    /**
     * @brief Get the name of the Vulkan result code
     * @return String representation of the VkResult enum
     */
    [[nodiscard]] const char *resultName() const noexcept
    {
        return vkResultToString(m_result);
    }

private:
    VkResult m_result;
    
    static std::string formatErrorMessage(VkResult result, std::string_view message) {
        std::string errorName = vkResultToString(result);
        return std::string(message) + " [" + errorName + " (" + std::to_string(result) + ")]";
    }
    
    static const char *vkResultToString(VkResult result)
    {
        switch (result)
        {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        default:
            return "UNKNOWN_VK_RESULT";
        }
    }
};

/**
 * @brief Memory allocation error
 */
class MemoryError : public ::std::runtime_error {
public:
    explicit MemoryError(std::string_view message)
        : ::std::runtime_error(std::string(message)) {
        LOG_ERROR("MemoryError: %s", what());
    }
};

/**
 * @brief Feature not supported error
 */
class UnsupportedFeatureError : public ::std::runtime_error {
public:
    explicit UnsupportedFeatureError(std::string_view message)
        : ::std::runtime_error(std::string(message)) {
        LOG_ERROR("UnsupportedFeatureError: %s", what());
    }
};

/**
 * @brief Check a Vulkan result and throw if not successful
 * 
 * @param result The Vulkan result to check
 * @param message Error message to include in the exception
 * @throws VulkanError if result is not VK_SUCCESS
 */
inline void check_result(VkResult result, std::string_view message) {
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan error: %s [%d]", message.data(), result);
        if (isDebugBuild)
            throw VulkanError(result, message);
    }
}

/**
 * @brief Check a condition and throw if false
 * 
 * @param condition Condition to check
 * @param message Error message if condition is false
 * @throws std::runtime_error if condition is false
 */
inline void check_condition(bool condition, std::string_view message) {
    if (!condition) {
        LOG_ERROR("Condition failed: %s", message.data());
        if (isDebugBuild)
            throw std::runtime_error(std::string(message));
    }
}

// Helper macro for Vulkan error checking
#define VK_CHECK(x)                                                 \
    VkResult err = x;                                               \
    if (err) {                                                      \
        std::string errorMsg = std::string("Vulkan error: ") +      \
                                std::to_string(err);                \
        LOG_ERROR("%s", errorMsg.c_str());                          \
        if(isDebugBuild) throw std::runtime_error(errorMsg);        \
    }                                                               \
   

} // namespace runtime

#endif // ERROR_HANDLING_H