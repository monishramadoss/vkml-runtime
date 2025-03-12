#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <vulkan/vulkan.h>
#include <string>
#include <string_view>
#include <stdexcept>

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
        : ::std::runtime_error(formatErrorMessage(result, message)), m_result(result) {}
    
    /**
     * @brief Get the Vulkan result code
     * @return The VkResult error code
     */
    [[nodiscard]] VkResult result() const noexcept { return m_result; }

    /**
     * @brief Get the name of the Vulkan result code
     * @return String representation of the VkResult enum
     */
    [[nodiscard]] const char* resultName() const noexcept;

private:
    VkResult m_result;
    
    static std::string formatErrorMessage(VkResult result, std::string_view message) {
        std::string errorName = vkResultToString(result);
        return std::string(message) + " [" + errorName + " (" + std::to_string(result) + ")]";
    }
    
    static const char* vkResultToString(VkResult result);
};

/**
 * @brief Memory allocation error
 */
class MemoryError : public ::std::runtime_error {
public:
    explicit MemoryError(std::string_view message)
        : ::std::runtime_error(std::string(message)) {}
};

/**
 * @brief Feature not supported error
 */
class UnsupportedFeatureError : public ::std::runtime_error {
public:
    explicit UnsupportedFeatureError(std::string_view message)
        : ::std::runtime_error(std::string(message)) {}
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
        throw std::runtime_error(std::string(message));
    }
}

// Helper macro for Vulkan error checking
#define VK_CHECK(x)                                                 \
    do {                                                            \
        VkResult err = x;                                           \
        if (err) {                                                  \
            std::string errorMsg = std::string("Vulkan error: ") +  \
                                  std::to_string(err);              \
            throw std::runtime_error(errorMsg);                   \
        }                                                           \
    } while (0)

// Helper class for error handling
class ErrorHandling {
public:
    static void checkVkResult(VkResult result) {
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Vulkan error: " + std::to_string(result));
        }
    }
};

} // namespace runtime

#endif // ERROR_HANDLING_H