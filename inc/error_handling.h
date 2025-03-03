#pragma once
#include <vulkan/vulkan.h>
#include <string_view>
#include <stdexcept>

namespace runtime {

class VulkanError : public std::runtime_error {
public:
    VulkanError(VkResult result, std::string_view message)
        : std::runtime_error(std::string(message)), m_result(result) {}
    
    VkResult result() const { return m_result; }
private:
    VkResult m_result;
};


inline void check_result(VkResult result, std::string_view message) {
    if (result != VK_SUCCESS) {
        throw VulkanError(result, message);
    }
}

} // namespace runtime
