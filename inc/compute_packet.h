#include <vulkan/vulkan.h>
#include <volk.h>
#include <vk_mem_alloc.h>

namespace runtime {
    struct ComputePacket {
        uint32_t queueFamilyIndex;
        VkCommandBuffer cmdBuffer;
        VkFence fence;
        VkSemaphore signalSemaphore;
        VkSemaphore waitSemaphore;
        VkSemaphare binaryDeviceSemaphore;        
    };
}