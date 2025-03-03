#pragma once
#include <vulkan/vulkan.h>

namespace runtime {

struct Version {
    static constexpr auto APPLICATION_NAME = "Hello ML";
    static constexpr auto ENGINE_NAME = "vkmlrt";
    static constexpr uint32_t APPLICATION_VERSION = VK_MAKE_VERSION(1, 0, 0);
    static constexpr uint32_t ENGINE_VERSION = VK_MAKE_VERSION(0, 0, 1);
    static constexpr uint32_t API_VERSION = VK_API_VERSION_1_3;
};

} // namespace runtime
